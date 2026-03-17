ESP_IDF_EXPORT ?= $(if $(IDF_PATH),$(IDF_PATH)/export.sh,$(HOME)/esp/esp-idf/export.sh)
PORT ?=
IDF_PY ?= idf.py

.PHONY: build flash monitor erase clean

build:
	bash -c "source $(ESP_IDF_EXPORT) && $(IDF_PY) build"

flash:
	bash -c "source $(ESP_IDF_EXPORT) && $(IDF_PY) $(if $(PORT),-p $(PORT),) flash"

monitor:
	bash -c "source $(ESP_IDF_EXPORT) && $(IDF_PY) $(if $(PORT),-p $(PORT),) monitor"

erase:
	bash -c "source $(ESP_IDF_EXPORT) && $(IDF_PY) $(if $(PORT),-p $(PORT),) erase-flash"

clean:
	bash -c "source $(ESP_IDF_EXPORT) && $(IDF_PY) fullclean"
