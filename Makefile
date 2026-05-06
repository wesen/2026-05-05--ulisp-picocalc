# uLisp PicoCalc — build and deploy

FQBN     := rp2040:rp2040:rpipico
SKETCH   := ulisp-picocalc-sketch
BUILD    := build
UF2      := $(BUILD)/ulisp-picocalc-sketch.ino.uf2
BIN      := $(BUILD)/ulisp-picocalc-sketch.ino.bin

# SD card upload target (Mac sharing PicoCalc SD)
SD_HOST  := manuel@192.168.0.57
SD_VOL   := /Volumes/NO NAME
SD_PATH  := $(SD_VOL)/firmware
SD_UF2   := PicoCalc_uLisp_4.8f.uf2
SD_BIN   := uLisp_4.8f.bin

.PHONY: build upload upload-uf2 upload-bin extract unmount clean

build: $(UF2)
	arduino-cli compile \
		--fqbn $(FQBN) \
		--build-path $(BUILD) \
		--warnings all \
		$(SKETCH)
	@echo "=== Build outputs ==="
	@ls -la $(UF2) $(BIN)

extract: $(UF2)
	python3 scripts/uf2-to-bin.py $(UF2) $(BIN)

upload: upload-uf2 upload-bin

upload-uf2: $(UF2)
	scp $(UF2) "$(SD_HOST):$(SD_PATH)/$(SD_UF2)"
	@echo "=== Uploaded UF2 to $(SD_HOST):$(SD_PATH)/$(SD_UF2) ==="

upload-bin: $(BIN)
	scp $(BIN) "$(SD_HOST):$(SD_PATH)/$(SD_BIN)"
	@echo "=== Uploaded BIN to $(SD_HOST):$(SD_PATH)/$(SD_BIN) ==="

unmount:
	ssh $(SD_HOST) 'sync; diskutil unmount "$(SD_VOL)"'
	@echo "=== Unmounted $(SD_VOL) on $(SD_HOST) ==="

clean:
	rm -rf $(BUILD)
