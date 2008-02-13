#pragma once
#ifndef FP_GLOBAL_H
#define FP_GLOBAL_H

#define FP_CLEAR_COLOR 0.0f, 0.05f, 0.15f, 0.55f
#define FP_MAX_LIGHTS 3
#define FP_OBJECT_RADIUS 20.0f
#define FP_DEFAULT_LIGHT_SCALE 1.0f

// For particle Init
#define FP_NUM_PARTICLES_X 12
#define FP_NUM_PARTICLES_Y 12
#define FP_NUM_PARTICLES_Z 12
#define FP_NUM_PARTICLES FP_NUM_PARTICLES_X * FP_NUM_PARTICLES_Y * FP_NUM_PARTICLES_Z

#define FP_PARTICLE_SPACING_X 1.0f
#define FP_PARTICLE_SPACING_Y FP_PARTICLE_SPACING_X
#define FP_PARTICLE_SPACING_Z FP_PARTICLE_SPACING_X


// For simulation
#define FP_TIME_FACTOR 2.5f
#define FP_SIMULATION_STEPS_PER_FRAME 1

#define FP_GLASS_RADIUS 12.0f
#define FP_GLASS_FLOOR -20.0f

#define FP_FLUID_DEFAULT_GRAVITY D3DXVECTOR3(0.0f, 1 * -9.81f, 0.0f)
#define FP_FLUID_DEFAULT_SMOOTHING_LENGTH 3.0f

#define FP_FLUID_DEFAULT_GAS_CONSTANT_K 300.0f
#define FP_FLUID_DEFAULT_SURFACE_TENSION 0.0015f
#define FP_FLUID_DEFAULT_GRADIENT_COLORFIELD_THRESHOLD 0.075f
#define FP_FLUID_DEFAULT_VISCOSITY 0.0015f
#define FP_FLUID_DEFAULT_PARTICLE_MASS 0.00020543f
#define FP_FLUID_DEFAULT_REST_DENSITY_COEFFICIENT 5.0f 
#define FP_FLUID_DEFAULT_DAMPING_COEFFICIENT 0.99f 

#define FP_FLUID_INITIAL_GRID_SIDELENGTH 50
#define FP_FLUID_INITIAL_GRID_CAPACITY FP_FLUID_INITIAL_GRID_SIDELENGTH \
        * FP_FLUID_INITIAL_GRID_SIDELENGTH * FP_FLUID_INITIAL_GRID_SIDELENGTH
#define FP_FLUID_INITIAL_PAIR_CACHE_CAPACITY FP_NUM_PARTICLES * 20 / 4 // d.o. num threads

// Controls the minimal distance that is used in glass-force-calculations
// Lower values lead to higher push-back forces
#define FP_FLUID_DEFAULT_GLASS_PUSHBACK_DISTANCE 0.6f

// Controls how far a particle can leave the glass until "manual collision handling"
// brings it back in
#define FP_FLUID_DEFAULT_GLASS_ENFORCE_DISTANCE 0.1f

// Controls "how many particles" are used to simulate the glass
#define FP_FLUID_DEFAULT_GLASS_DENSITY 2.0f
#define FP_FLUID_DEFAULT_GLASS_VISCOSITY 1.2f


// For simulation debug:
#define FP_DEBUG_MAX_FORCE 0.5f
#define FP_DEBUG_MAX_FORCE_SQ FP_DEBUG_MAX_FORCE * FP_DEBUG_MAX_FORCE


// For rendering

// Sprites
#define FP_RENDER_DEFAULT_SPRITE_SIZE 1.0f

// Marching cubes
#define FP_MC_DEFAULT_INITIAL_CELL_CAPACITY 10
#define FP_MC_INITIAL_GRID_SIDELENGTHFLUID_MAX_POS 150.0f
#define FP_MC_DEFAULT_ISOVOLUME_VOXELSIZE 0.9f
#define FP_MC_DEFAULT_ISO_VOLUME_BORDER 5.0f
#define FP_MC_DEFAULT_ISO_LEVEL 0.024f
#define FP_MC_INITIAL_ISOVOLUME_SIDELENGTH 200
#define FP_MC_MAX_TRIANGLES 1000000
#define FP_MC_MAX_VETICES FP_MC_MAX_TRIANGLES * 3
#define FP_MC_ISO_VOLUME_GROW_FACTOR 0.2f
#define FP_MC_ISO_VOLUME_SHRINK_BORDER 0.8f
#define FP_MC_INITIAL_ISOVOLUME_CAPACITY FP_MC_INITIAL_ISOVOLUME_SIDELENGTH \
        * FP_MC_INITIAL_ISOVOLUME_SIDELENGTH * FP_MC_INITIAL_ISOVOLUME_SIDELENGTH

// Raycast
#define FP_RAYCAST_DEFAULT_ISO_LEVEL       0.16f
#define FP_RAYCAST_DEFAULT_STEP_SCALE      2.0f //0.8f
#define FP_RAYCAST_VOLUME_TEXTURE_WIDTH   64
#define FP_RAYCAST_VOLUME_TEXTURE_HEIGHT 128
#define FP_RAYCAST_VOLUME_TEXTURE_DEPTH   64
#define FP_RAYCAST_VOLUME_TEXTURE_DIMENSIONS fp_VolumeIndex(\
        FP_RAYCAST_VOLUME_TEXTURE_WIDTH, FP_RAYCAST_VOLUME_TEXTURE_HEIGHT,\
        FP_RAYCAST_VOLUME_TEXTURE_DEPTH);
#define FP_RAYCAST_VOLUME_BORDER 5.0f
#define FP_RAYCAST_DEFAULT_VOXEL_SIZE 2.0f * (FP_GLASS_RADIUS + FP_RAYCAST_VOLUME_BORDER)\
        / FP_RAYCAST_VOLUME_TEXTURE_WIDTH
#define FP_RAYCAST_DEFAULT_REFRACTION_RATIO 0.88f

#endif
