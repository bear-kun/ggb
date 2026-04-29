#include "cgraph/graph.h"
#include "cgraph/iter.h"
#include "graph.h"
#include <stdlib.h>

typedef enum { NODE_VALUE = 1, NODE_COMPUTE = 2 } NodeType;

typedef struct {
  NodeType type;
  GeomInt ref_count;

  unsigned version;
  float value;
  int soln_count; // solution count
  ValueEval eval;

  GeomInt indegree; // sub-graph
} GraphNode;

typedef struct {
  GeomSize front, rear;
  GeomId *elems;
} Queue;

static struct {
  CGraph graph;
  GraphNode *nodes;
  Queue queue;
} intl;

static void graph_vert_resize_callback(CGraphSize, CGraphSize);
static bool graph_get_inputs(GeomId id, float *inputs);
static bool graph_link_inputs(GeomId id, GeomSize input_size, const GeomId *inputs,
                              float *input_values);
static void graph_init_indegree(Queue *queue);

static void enqueue(Queue *q, const GeomId id) { q->elems[q->rear++] = id; }
static GeomId dequeue(Queue *q) { return q->elems[q->front++]; }
static void queue_clear(Queue *q) { q->front = q->rear = 0; }
static bool queue_empty(const Queue *q) { return q->front == q->rear; }

void computation_graph_init(const GeomSize init_size) {
  intl.nodes = malloc(init_size * sizeof(GraphNode));
  intl.queue.elems = malloc(init_size * sizeof(GeomId));

  cgraphInit(&intl.graph, true, init_size, init_size);
  cgraphSetVertResizeCallback(&intl.graph, graph_vert_resize_callback);
}

void computation_graph_cleanup() {
  free(intl.nodes);
  free(intl.queue.elems);
  cgraphRelease(&intl.graph);
}

void computation_graph_clear() {
  cgraphClear(&intl.graph);
}

GeomId graph_add_value(const float value) {
  const GeomId id = cgraphAddVert(&intl.graph);
  GraphNode *node = intl.nodes + id;
  node->type = NODE_VALUE;
  node->ref_count = 0;

  node->version = 0;
  node->value = value;
  node->soln_count = 1;
  node->eval = NULL;

  node->indegree = 0;
  return id;
}

GeomId graph_add_constraint(const GeomSize input_size, const GeomId *inputs,
                            const GeomSize output_size, const GeomId *outputs,
                            const ValueEval eval) {
  const GeomId node_id = cgraphAddVert(&intl.graph);
  GraphNode *node = intl.nodes + node_id;
  node->type = NODE_COMPUTE;
  node->ref_count = 0;
  node->eval = eval;
  node->indegree = 0;

  float input_values[6], output_values[6];
  if (graph_link_inputs(node_id, input_size, inputs, input_values)) {
    node->soln_count = eval(input_values, output_values);
  } else {
    node->soln_count = 0;
  }

  for (GeomSize i = output_size; i--;) {
    const GeomId output_id = outputs[i];
    cgraphAddEdge(&intl.graph, node_id, output_id);

    GraphNode *out_node = intl.nodes + output_id;
    out_node->value = output_values[i];
    out_node->soln_count = node->soln_count != 0;
    out_node->ref_count++;
  }
  return node_id;
}

unsigned graph_get_version(const GeomSize count, const GeomId *ids) {
  unsigned version = 0;
  for (GeomSize i = 0; i < count; i++) {
    const GraphNode *node = intl.nodes + ids[i];
    if (node->version > version) version = node->version;
  }
  return version;
}

bool graph_get_values(const GeomSize count, const GeomId *ids, float *values) {
  for (GeomSize i = 0; i < count; i++) {
    const GraphNode *node = intl.nodes + ids[i];
    if (node->soln_count == 0) return false;
    values[i] = node->value;
  }
  return true;
}

bool graph_is_degenerate(const GeomId constr, const GeomId soln_id) {
  return soln_id >= intl.nodes[constr].soln_count;
}

void graph_ref(const GeomId id) { intl.nodes[id].ref_count++; }

void graph_unref(const GeomId id) {
  GraphNode *node = intl.nodes + id;
  if (--node->ref_count > 0) return;

  if (node->type == NODE_COMPUTE) {
    CGraphId from, eid, to;
    CGraphIter iter = cgraphGetEdgeIter(&intl.graph, id, CGRAPH_OUT);
    while (cgraphIterNextEdge(&iter, &eid, &to)) {
      cgraphDeleteEdge(&intl.graph, eid);
      graph_unref(to);
    }

    iter = cgraphGetEdgeIter(&intl.graph, id, CGRAPH_IN);
    while (cgraphIterNextEdge(&iter, &eid, &from)) {
      cgraphDeleteEdge(&intl.graph, eid);
      graph_unref(from);
    }
  }

  cgraphDeleteVert(&intl.graph, id);
}

void graph_change_value(const GeomSize count, const GeomId *ids, const float *values) {
  static Queue *queue = &intl.queue;

  queue_clear(queue);
  for (GeomSize i = 0; i < count; i++) {
    GraphNode *node = intl.nodes + ids[i];
    node->value = values[i];
    enqueue(queue, ids[i]);
  }

  graph_init_indegree(queue);
  while (!queue_empty(queue)) {
    const GeomId id = dequeue(queue);
    GraphNode *node = intl.nodes + id;
    node->version++;

    CGraphId eid, to;
    CGraphIter iter = cgraphGetEdgeIter(&intl.graph, id, CGRAPH_OUT);
    switch (node->type) {
    case NODE_COMPUTE: {
      float inputs[6], outputs[6];
      if (graph_get_inputs(id, inputs)) {
        node->soln_count = node->eval(inputs, outputs);
      } else {
        node->soln_count = 0;
      }

      int i = 0;
      while (cgraphIterNextEdge(&iter, &eid, &to)) {
        GraphNode *out_node = intl.nodes + to;
        out_node->value = outputs[i++];
        out_node->soln_count = node->soln_count != 0;
        if (--out_node->indegree == 0) enqueue(&intl.queue, to);
      }
      break;
    }
    case NODE_VALUE:
      while (cgraphIterNextEdge(&iter, &eid, &to)) {
        if (--intl.nodes[to].indegree == 0) enqueue(&intl.queue, to);
      }
    }
  }
}

static void graph_vert_resize_callback(const CGraphSize old_cap, const CGraphSize new_cap) {
  void *mem = realloc(intl.nodes, new_cap * sizeof(GraphNode));
  if (!mem) abort();
  intl.nodes = mem;
  mem = realloc(intl.queue.elems, new_cap * sizeof(GeomId));
  if (!mem) abort();
  intl.queue.elems = mem;
}

static bool graph_link_inputs(const GeomId id, const GeomSize input_size,
                              const GeomId *inputs, float *input_values) {
  bool valid = true;
  for (GeomSize i = input_size; i--;) {
    const GeomId input_id = inputs[i];
    cgraphAddEdge(&intl.graph, input_id, id);

    GraphNode *node = intl.nodes + input_id;
    input_values[i] = node->value;
    node->ref_count++;
    if (node->soln_count == 0) valid = false;
  }
  return valid;
}

static bool graph_get_inputs(const GeomId id, float *inputs) {
  CGraphId from, eid;
  CGraphIter iter = cgraphGetEdgeIter(&intl.graph, id, CGRAPH_IN);
  while (cgraphIterNextEdge(&iter, &eid, &from)) {
    if (intl.nodes[from].soln_count == 0) return false;
    *inputs++ = intl.nodes[from].value;
  }
  return true;
}

static void graph_init_indegree(Queue *queue) {
  const GeomSize count = queue->rear;

  while (!queue_empty(queue)) {
    const GeomId id = dequeue(queue);

    CGraphId eid, to;
    CGraphIter iter = cgraphGetEdgeIter(&intl.graph, id, CGRAPH_OUT);
    while (cgraphIterNextEdge(&iter, &eid, &to)) {
      GraphNode *node = intl.nodes + to;
      if (node->indegree++ == 0) enqueue(queue, to);
    }
  }
  queue->front = 0;
  queue->rear = count;
}