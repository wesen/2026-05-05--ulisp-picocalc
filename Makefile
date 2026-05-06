# uLisp PicoCalc — build and deploy

FQBN     := rp2040:rp2040:rpipico
SKETCH   := ulisp-picocalc-sketch
BUILD    := build
UF2      := $(BUILD)/ulisp-picocalc-sketch.ino.uf2

# SD card upload target (Mac sharing PicoCalc SD)
SD_HOST  := manuel@192.168.0.57
SD_PATH  := '/Volumes/NO NAME/firmware'
SD_NAME  := PicoCalc_uLisp_4.8f.uf2

.PHONY: build upload clean

build: $(UF2)

$(UF2): $(SKETCH)/$(SKETCH).ino
	mkdir -p $(BUILD)
	arduino-cli compile \
		--fqbn $(FQBN) \
		--build-path $(BUILD) \
		--warnings all \
		$(SKETCH)
	@echo "=== Build: $(UF2) ==="
	@ls -la $(UF2)

upload: $(UF2)
	scp $(UF2) '$(SD_HOST):$(SD_PATH)/$(SD_NAME)'
	@echo "=== Uploaded to $(SD_HOST):$(SD_PATH)/$(SD_NAME) ==="

clean:
	rm -rf $(BUILD)
