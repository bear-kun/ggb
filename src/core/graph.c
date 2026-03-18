#include "cgraph/graph.h"
#include "cgraph/iter.h"
#include "graph.h"

#include <stdlib.h>
#include <string.h>

typedef enum { NODE_VALUE = 1, NODE_COMPUTE = 2 } NodeType;

typedef struct {
  NodeType type;
  float value;
  int soln_count; // solution count
  GeomInt ref_count;

  GeomInt indegree; // sub-graph
  GeomId dep_head; // eid
  ValueEval eval;
} GraphNode;

typedef struct {
  GeomSize front, rear;
  GeomId *elems;
} Queue;

static struct {
  CGraph graph;
  GraphNode *nodes;
  GeomId *dep_next;
  Queue queue;
} internal;

static void graph_vert_resize_callback(CGraphSize, CGraphSize);
static void graph_edge_resize_callback(CGraphSize, CGraphSize);
static bool graph_get_inputs(const GraphNode *node, float *inputs);
static bool graph_link_inputs(GeomId id, GeomSize input_size,
                              const GeomId *inputs, float *input_values);
static void graph_init_indegree(Queue *queue, CGraphIter *iter);

static void enqueue(Queue *q, const GeomId id) { q->elems[q->rear++] = id; }
static GeomId dequeue(Queue *q) { return q->elems[q->front++]; }
static void queue_clear(Queue *q) { q->front = q->rear = 0; }
static bool queue_empty(const Queue *q) { return q->front == q->rear; }

void computation_graph_init(const GeomSize init_size) {
  internal.nodes = malloc(init_size * sizeof(GraphNode));
  internal.dep_next = malloc(init_size * sizeof(GeomId));
  internal.queue.elems = malloc(init_size * sizeof(GeomId));
  memset(internal.dep_next, -1, init_size * sizeof(GeomId));

  cgraphInit(&internal.graph, true, init_size, init_size);
  cgraphSetVertResizeCallback(&internal.graph, graph_vert_resize_callback);
  cgraphSetEdgeResizeCallback(&internal.graph, graph_edge_resize_callback);
}

void computation_graph_cleanup() {
  free(internal.nodes);
  free(internal.dep_next);
  free(internal.queue.elems);
  cgraphRelease(&internal.graph);
}

void computation_graph_clear() {
  memset(internal.dep_next, -1, internal.graph.edgeCap * sizeof(GeomId));
  cgraphClear(&internal.graph);
}

GeomId graph_add_value(const float value) {
  const GeomId id = (GeomId)cgraphAddVert(&internal.graph);
  GraphNode *node = internal.nodes + id;
  node->type = NODE_VALUE;
  node->soln_count = 1;
  node->value = value;
  node->ref_count = 0;
  node->indegree = 0;
  node->dep_head = -1;
  node->eval = NULL;
  return id;
}

GeomId graph_add_constraint(const GeomSize input_size, const GeomId *inputs,
                            const GeomSize output_size, const GeomId *outputs,
                            const ValueEval eval) {
  const GeomId node_id = (GeomId)cgraphAddVert(&internal.graph);
  GraphNode *node = internal.nodes + node_id;
  node->type = NODE_COMPUTE;
  node->ref_count = 0;
  node->indegree = 0;
  node->eval = eval;

  float input_values[6], output_values[6];
  if (graph_link_inputs(node_id, input_size, inputs, input_values)) {
    node->soln_count = eval(input_values, output_values);
  } else {
    node->soln_count = 0;
  }

  for (GeomSize i = 0; i < output_size; i++) {
    const GeomId output_id = outputs[i];
    const GeomId eid =
        (GeomId)cgraphPushEdgeBack(&internal.graph, node_id, output_id);

    GraphNode *out_node = internal.nodes + output_id;
    out_node->value = output_values[i];
    out_node->soln_count = node->soln_count != 0;
    out_node->dep_head = eid;
    out_node->ref_count++;
  }
  return node_id;
}

float graph_get_value(const GeomId id) { return internal.nodes[id].value; }

bool graph_is_valid(const GeomSize count, const GeomId *ids) {
  for (GeomSize i = 0; i < count; i++) {
    if (internal.nodes[ids[i]].soln_count == 0) return false;
  }
  return true;
}

bool graph_is_degenerate(const GeomId constr, const GeomId soln_id) {
  return soln_id >= internal.nodes[constr].soln_count;
}

void graph_ref(const GeomId id) { internal.nodes[id].ref_count++; }

void graph_unref(const GeomId id) {
  GraphNode *node = internal.nodes + id;
  if (--node->ref_count > 0) return;

  if (node->type == NODE_COMPUTE) {
    CGraphIter *iter = cgraphGetIter(&internal.graph);
    CGraphId eid, to;
    while (cgraphIterNextEdge(iter, id, &eid, &to)) {
      cgraphDeleteEdge(&internal.graph, eid);
      graph_unref((GeomId)to);
    }
    cgraphIterRelease(iter);

    for (GeomId dep = node->dep_head; dep != -1; dep = internal.dep_next[dep]) {
      CGraphId from;
      cgraphParseEdgeId(&internal.graph, dep, &from, NULL);
      cgraphDeleteEdge(&internal.graph, dep);
      graph_unref((GeomId)from);
    }
  }

  cgraphDeleteVert(&internal.graph, id);
}

void graph_change_value(const GeomSize count, const GeomId *ids,
                        const float *values) {
  Queue *queue = &internal.queue;
  CGraphIter *iter = cgraphGetIter(&internal.graph);

  queue_clear(queue);
  for (GeomSize i = 0; i < count; i++) {
    enqueue(queue, ids[i]);
    internal.nodes[ids[i]].value = values[i];
  }

  graph_init_indegree(queue, iter);
  while (!queue_empty(queue)) {
    const GeomId id = dequeue(queue);
    GraphNode *node = internal.nodes + id;

    CGraphId eid, to;
    cgraphIterResetEdge(iter, id);
    switch (node->type) {
    case NODE_COMPUTE: {
      float inputs[6], outputs[6];
      if (graph_get_inputs(node, inputs)) {
        node->soln_count = node->eval(inputs, outputs);
      } else {
        node->soln_count = 0;
      }

      int i = 0;
      while (cgraphIterNextEdge(iter, id, &eid, &to)) {
        GraphNode *out_node = internal.nodes + to;
        out_node->value = outputs[i++];
        out_node->soln_count = node->soln_count != 0;
        if (--out_node->indegree == 0) {
          enqueue(&internal.queue, (GeomId)to);
        }
      }
      break;
    }
    case NODE_VALUE:
      while (cgraphIterNextEdge(iter, id, &eid, &to)) {
        if (--internal.nodes[to].indegree == 0) {
          enqueue(&internal.queue, (GeomId)to);
        }
      }
    }
  }

  cgraphIterRelease(iter);
}

static void graph_vert_resize_callback(const CGraphSize old_cap,
                                       const CGraphSize new_cap) {
  void *mem = realloc(internal.nodes, new_cap * sizeof(GraphNode));
  if (!mem) abort();
  internal.nodes = mem;
  mem = realloc(internal.queue.elems, new_cap * sizeof(GeomId));
  if (!mem) abort();
  internal.queue.elems = mem;
}

static void graph_edge_resize_callback(const CGraphSize old_cap,
                                       const CGraphSize new_cap) {
  void *mem = realloc(internal.dep_next, new_cap * sizeof(GeomId));
  if (!mem) abort();
  internal.dep_next = mem;
  memset(internal.dep_next + old_cap, -1, (new_cap - old_cap) * sizeof(GeomId));
}

static bool graph_link_inputs(const GeomId id, const GeomSize input_size,
                              const GeomId *inputs, float *input_values) {
  bool valid = true;
  GeomId *list_tail = &internal.nodes[id].dep_head;
  for (GeomSize i = 0; i < input_size; i++) {
    const GeomId input_id = inputs[i];

    const GeomId eid = (GeomId)cgraphAddEdge(&internal.graph, input_id, id);
    *list_tail = eid;
    list_tail = internal.dep_next + eid;

    GraphNode *node = internal.nodes + input_id;
    input_values[i] = node->value;
    node->ref_count++;
    if (node->soln_count == 0) valid = false;
  }
  return valid;
}

static bool graph_get_inputs(const GraphNode *node, float *inputs) {
  for (CGraphId dep = node->dep_head; dep != -1; dep = internal.dep_next[dep]) {
    CGraphId from;
    cgraphParseEdgeId(&internal.graph, dep, &from, NULL);
    if (internal.nodes[from].soln_count == 0) return false;
    *inputs++ = internal.nodes[from].value;
  }
  return true;
}

static void graph_init_indegree(Queue *queue, CGraphIter *iter) {
  const GeomSize count = queue->rear;
  bool *visited = calloc(internal.graph.vertCap, sizeof(bool));

  while (!queue_empty(queue)) {
    const GeomId id = dequeue(queue);
    visited[id] = true;

    CGraphId eid, to;
    while (cgraphIterNextEdge(iter, id, &eid, &to)) {
      internal.nodes[to].indegree++;
      if (!visited[to]) {
        visited[to] = true;
        enqueue(queue, (GeomId)to);
      }
    }
  }
  queue->front = 0;
  queue->rear = count;
  free(visited);
}