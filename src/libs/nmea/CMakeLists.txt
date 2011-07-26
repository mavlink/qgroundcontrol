INCLUDE_DIRECTORIES(include)

SET_SOURCE_FILES(NMEALIB_SRC_FILES
  context.c
  time.c
  generate.c
  generator.c
  gmath.c
  info.c
  parse.c
  parser.c
  sentence.c
  tok.c
)

PIXHAWK_LIBRARY(nmea SHARED ${NMEALIB_SRC_FILES})

