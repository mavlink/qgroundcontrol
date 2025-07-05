# Basic appearance ------------------------------------------------
set(GRAPHVIZ_GRAPH_NAME      "QGC-deps")          # title
set(GRAPHVIZ_GRAPH_HEADER    "rankdir=LR;")       # dot header snippet
set(GRAPHVIZ_NODE_PREFIX     "node")              # node IDs

# What to show ----------------------------------------------------
set(GRAPHVIZ_EXECUTABLES     TRUE)   # default TRUE
set(GRAPHVIZ_SHARED_LIBS     TRUE)
set(GRAPHVIZ_MODULE_LIBS     FALSE)
set(GRAPHVIZ_INTERFACE_LIBS  FALSE)
set(GRAPHVIZ_OBJECT_LIBS     FALSE)
set(GRAPHVIZ_UNKNOWN_LIBS    FALSE)
set(GRAPHVIZ_EXTERNAL_LIBS   FALSE)  # hide Qt/system libs
set(GRAPHVIZ_CUSTOM_TARGETS  TRUE)   # include custom targets
set(GRAPHVIZ_IGNORE_TARGETS  "test_.*;doc_.*")   # regex list

# File churn ------------------------------------------------------
set(GRAPHVIZ_GENERATE_PER_TARGET   FALSE)  # only the master graph
set(GRAPHVIZ_GENERATE_DEPENDERS    FALSE)

# --graphviz=foo.dot
