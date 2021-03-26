#!/bin/sh

cd src/main &&
hg clone https://hg.libsdl.org/SDL/ &&
for r in ctf rogue xatrix yquake2; do
	git clone https://github.com/yquake2/${r}
done &&
patch -p1 < yquake2.patch &&
cd ../../.. &&
patch -p1 < Quake2VR/cardboard.patch &&
cd Quake2VR &&
echo YAYYYYYYYYY

