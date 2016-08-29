# -*- Mode: CMake -*-
#
# CMake build file for the Printer algorithm
#

# Production specification for the Printer algorithm
#
add_algorithm(Printer Printer.cc)

target_link_libraries(Printer)

add_unit_test(PrinterTest.cc Printer)
