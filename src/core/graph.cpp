#include "cgraph/graph.h"
#include "cgraph/iterator.h"
#include "internal.hpp"
#include <queue>

namespace geom::graph {
enum NodeType { NODE_VALUE = 1, NODE_COMPUTE = 2 };

struct GraphNode {
  NodeType type;
  float value;
  EvalType eval;
  int ref_count;
  unsigned version;
  int soln_count; // solution count
  int indegree; // sub-graph
};

static struct {
  CGraph graph{};
  std::vector<GraphNode> nodes;
} intl;

void init(const GeomSize init_size) {
  intl.nodes.resize(init_size);
  cgraph_init(&intl.graph, true, init_size, init_size);
}

void cleanup() {
  intl.nodes.clear();
  cgraph_release(&intl.graph);
}

void clear() {
  cgraph_clear(&intl.graph);
}

static GeomId get_new_id() {
  const GeomId id = cgraph_add_vertex(&intl.graph);
  if (id >= intl.nodes.size()) intl.nodes.resize(intl.graph.vert.capacity);
  return id;
}

GeomId add_value(const float value) {
  const GeomId id = get_new_id();
  GraphNode &node = intl.nodes[id];
  node.type = NODE_VALUE;
  node.value = value;
  node.eval = EVAL_NULL;
  node.ref_count = 0;

  node.version = 1;
  node.soln_count = 1;
  node.indegree = 0;
  return id;
}

static bool link_inputs(const GeomId id, const GeomSize input_size,
                        const GeomId *inputs, float *input_values) {
  bool valid = true;
  for (GeomSize i = input_size; i--;) {
    const GeomId input_id = inputs[i];
    cgraph_add_edge(&intl.graph, input_id, id);

    GraphNode &node = intl.nodes[input_id];
    input_values[i] = node.value;
    node.ref_count++;
    if (node.soln_count == 0) valid = false;
  }
  return valid;
}

GeomId add_constraint(const GeomSize input_size, const GeomId *inputs,
                      const GeomSize output_size, const GeomId *outputs, const EvalType eval) {
  const GeomId node_id = get_new_id();;
  GraphNode &node = intl.nodes[node_id];
  node.type = NODE_COMPUTE;
  node.eval = eval;
  node.ref_count = 0;
  node.indegree = 0;

  float input_values[6], output_values[6];
  if (link_inputs(node_id, input_size, inputs, input_values)) {
    node.soln_count = eval_map[eval](input_values, output_values);
  } else {
    node.soln_count = 0;
  }

  for (GeomSize i = output_size; i--;) {
    const GeomId output_id = outputs[i];
    cgraph_add_edge(&intl.graph, node_id, output_id);

    GraphNode &out_node = intl.nodes[output_id];
    out_node.value = output_values[i];
    out_node.soln_count = node.soln_count != 0;
    out_node.ref_count++;
  }
  return node_id;
}

unsigned get_version(const GeomSize count, const GeomId *ids) {
  unsigned version = 0;
  for (GeomSize i = 0; i < count; i++) {
    const GraphNode &node = intl.nodes[ids[i]];
    if (node.version > version) version = node.version;
  }
  return version;
}

bool get_values(const GeomSize count, const GeomId *ids, float *values) {
  for (GeomSize i = 0; i < count; i++) {
    const GraphNode &node = intl.nodes[ids[i]];
    if (node.soln_count == 0) return false;
    values[i] = node.value;
  }
  return true;
}

bool is_degenerate(const GeomId constr, const GeomId soln_id) {
  return soln_id >= intl.nodes[constr].soln_count;
}

void ref_node(const GeomId id) { intl.nodes[id].ref_count++; }

void unref_node(const GeomId id) {
  GraphNode &node = intl.nodes[id];
  if (--node.ref_count > 0) return;

  if (node.type == NODE_COMPUTE) {
    CGraphId from, eid, to;
    CGraphIterator iter = cgraph_get_edge_iterator(&intl.graph, id, CGRAPH_OUT);
    while (cgraph_iterator_next_edge(&iter, &eid, &to)) {
      cgraph_delete_edge(&intl.graph, eid);
      unref_node(to);
    }

    iter = cgraph_get_edge_iterator(&intl.graph, id, CGRAPH_IN);
    while (cgraph_iterator_next_edge(&iter, &eid, &from)) {
      cgraph_delete_edge(&intl.graph, eid);
      unref_node(from);
    }
  }

  cgraph_delete_vertex(&intl.graph, id);
}

static void init_indegree(std::queue<GeomId> queue) {
  while (!queue.empty()) {
    const GeomId id = queue.front();
    queue.pop();

    CGraphId eid, to;
    CGraphIterator iter = cgraph_get_edge_iterator(&intl.graph, id, CGRAPH_OUT);
    while (cgraph_iterator_next_edge(&iter, &eid, &to)) {
      if (intl.nodes[to].indegree++ == 0) queue.push(to);
    }
  }
}

static bool get_inputs(const GeomId id, float *inputs) {
  CGraphId from, eid;
  CGraphIterator iter = cgraph_get_edge_iterator(&intl.graph, id, CGRAPH_IN);
  while (cgraph_iterator_next_edge(&iter, &eid, &from)) {
    if (intl.nodes[from].soln_count == 0) return false;
    *inputs++ = intl.nodes[from].value;
  }
  return true;
}

void change_value(const GeomSize count, const GeomId *ids, const float *values) {
  std::queue<GeomId> queue = {};
  CGraph &graph = intl.graph;

  for (GeomSize i = 0; i < count; i++) {
    GraphNode &node = intl.nodes[ids[i]];
    node.value = values[i];
    queue.push(ids[i]);
  }

  init_indegree(queue);
  while (!queue.empty()) {
    const GeomId id = queue.front();
    queue.pop();

    GraphNode &node = intl.nodes[id];
    node.version++;

    CGraphId eid, to;
    CGraphIterator iter = cgraph_get_edge_iterator(&intl.graph, id, CGRAPH_OUT);
    switch (node.type) {
    case NODE_COMPUTE: {
      float inputs[6], outputs[6];
      if (get_inputs(id, inputs)) {
        node.soln_count = eval_map[node.eval](inputs, outputs);
      } else {
        node.soln_count = 0;
      }

      int i = 0;
      while (cgraph_iterator_next_edge(&iter, &eid, &to)) {
        GraphNode &out_node = intl.nodes[to];
        out_node.value = outputs[i++];
        out_node.soln_count = node.soln_count != 0;
        if (--out_node.indegree == 0) queue.push(to);
      }
      break;
    }
    case NODE_VALUE:
      while (cgraph_iterator_next_edge(&iter, &eid, &to)) {
        if (--intl.nodes[to].indegree == 0) queue.push(to);
      }
    }
  }
}
}