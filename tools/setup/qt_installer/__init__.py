"""QGC's Qt installer: download.qt.io online repository to an aqt-compatible install tree.

Standard library plus ``cmake -E tar``, so ``install_qt.py`` keeps running with a bare ``python3``.
``repo`` resolves Updates.xml paths and archives, ``download`` fetches them against checksums from
download.qt.io, ``install`` extracts and records the tree, ``patch`` applies aqtinstall's
post-install edits, and ``compare`` diffs an aqt tree against this installer's.
"""
