PLUGIN_NAME = spike_predictor

HEADERS = spike_predictor.h

SOURCES = spike_predictor.cpp\
          moc_spike_predictor.cpp\

LIBS = 

### Do not edit below this line ###

include $(shell rtxi_plugin_config --pkgdata-dir)/Makefile.plugin_compile
