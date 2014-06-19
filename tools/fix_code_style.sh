#!/bin/sh
astyle \
    --style=allman		\
    --indent=spaces=4	\
    --indent-cases		\
    --indent-preprocessor	\
    --break-blocks=all		\
    --pad-oper			\
    --pad-header		\
    --unpad-paren		\
    --keep-one-line-blocks	\
    --keep-one-line-statements	\
    --align-pointer=name	\
    --align-reference=name	\
    --suffix=none		\
    --ignore-exclude-errors-x	\
    --lineend=linux		\
    --exclude=EASTL		\
    $*
