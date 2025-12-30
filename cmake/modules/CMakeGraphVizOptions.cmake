# ============================================================================
# CMakeGraphVizOptions.cmake
# Configuration for CMake's --graphviz dependency graph generation
# Usage: cmake --graphviz=deps.dot . && dot -Tpng deps.dot -o deps.png
# ============================================================================

# ----------------------------------------------------------------------------
# Graph Appearance
# ----------------------------------------------------------------------------
set(GRAPHVIZ_GRAPH_NAME      "QGC-deps")          # Graph title
set(GRAPHVIZ_GRAPH_HEADER    "rankdir=LR;")       # DOT header snippet (left-to-right)
set(GRAPHVIZ_NODE_PREFIX     "node")              # Node ID prefix

# ----------------------------------------------------------------------------
# What to Include in Graph
# ----------------------------------------------------------------------------
set(GRAPHVIZ_EXECUTABLES     TRUE)   # Show executables (default TRUE)
set(GRAPHVIZ_SHARED_LIBS     TRUE)   # Show shared libraries
set(GRAPHVIZ_MODULE_LIBS     FALSE)  # Hide module libraries
set(GRAPHVIZ_INTERFACE_LIBS  FALSE)  # Hide interface libraries
set(GRAPHVIZ_OBJECT_LIBS     FALSE)  # Hide object libraries
set(GRAPHVIZ_UNKNOWN_LIBS    FALSE)  # Hide unknown libraries
set(GRAPHVIZ_EXTERNAL_LIBS   FALSE)  # Hide external libraries (Qt/system libs)
set(GRAPHVIZ_CUSTOM_TARGETS  TRUE)   # Show custom targets
set(GRAPHVIZ_IGNORE_TARGETS  "test_.*;doc_.*")   # Regex list of targets to ignore

# ----------------------------------------------------------------------------
# Output File Generation
# ----------------------------------------------------------------------------
set(GRAPHVIZ_GENERATE_PER_TARGET   FALSE)  # Only generate master graph
set(GRAPHVIZ_GENERATE_DEPENDERS    FALSE)  # Don't generate depender graphs
