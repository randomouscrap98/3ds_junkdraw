.PHONY: all

MYGAME=3ds_junkdraw

all: cxitool makerom ../${MYGAME}.3dsx
	./cxitool ../${MYGAME}.3dsx ${MYGAME}.cxi
	./makerom -v -f cia -o ${MYGAME}.cia -target t -i ${MYGAME}.cxi:0:0 # -ignoresig
	echo "Done"

cxitool: 3dstools
	cd 3dstools && git switch cxi-stuff
	cd 3dstools && ./autogen.sh && ./configure && make
	cp 3dstools/cxitool .

3dstools:
	git clone https://github.com/devkitPro/3dstools.git

makerom: Project_CTR
	cd Project_CTR/makerom && make deps && make
	cp Project_CTR/makerom/bin/makerom .

Project_CTR:
	git clone https://github.com/3DSGuy/Project_CTR.git

# Build the game if it doesn't exist
../${MYGAME}.3dsx:
	cd ../ && make
