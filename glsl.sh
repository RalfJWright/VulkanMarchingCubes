#!/bin/bash
pdir=~/projects/renderingnew
generation=isosurface_generation
meshing=isosurface_meshing
sname=voxel

#Compute
/usr/bin/glslc --target-spv=spv1.6 $pdir/src/gpu/shaders/$generation.comp -o $pdir/spv/$generation.comp.spv && echo "Compiled compute."
/usr/bin/glslc --target-spv=spv1.6 $pdir/src/gpu/shaders/$meshing.comp -o $pdir/spv/$meshing.comp.spv && echo "Compiled compute."

#Raster
/usr/bin/glslc --target-spv=spv1.6 $pdir/src/gpu/shaders/raster/$sname.vert -o $pdir/spv/$sname.vert.spv && echo "Compiled vertex."
/usr/bin/glslc --target-spv=spv1.6 $pdir/src/gpu/shaders/raster/$sname.frag -o $pdir/spv/$sname.frag.spv && echo "Compiled fragment."