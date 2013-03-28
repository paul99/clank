// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_TILE_MANAGER_H_
#define CC_TILE_MANAGER_H_

#include <list>
#include <queue>
#include <vector>

#include "base/hash_tables.h"
#include "base/memory/scoped_ptr.h"
#include "base/values.h"
#include "cc/resource_pool.h"
#include "cc/tile_priority.h"

namespace cc {

class RasterThread;
class ResourceProvider;
class Tile;
class TileVersion;
struct RenderingStats;

class CC_EXPORT TileManagerClient {
 public:
  virtual void ScheduleManageTiles() = 0;
  virtual void ScheduleCheckForCompletedSetPixels() = 0;

 protected:
  virtual ~TileManagerClient() {}
};

// Tile manager classifying tiles into a few basic
// bins:
enum TileManagerBin {
  NOW_BIN = 0, // Needed ASAP.
  SOON_BIN = 1, // Impl-side version of prepainting.
  EVENTUALLY_BIN = 2, // Nice to have, if we've got memory and time.
  NEVER_BIN = 3, // Dont bother.
  NUM_BINS = 4
};

// This is state that is specific to a tile that is
// managed by the TileManager.
class CC_EXPORT ManagedTileState {
 public:
  ManagedTileState();
  ~ManagedTileState();

  // Persisted state: valid all the time.
  bool can_use_gpu_memory;
  bool can_be_freed;
  scoped_ptr<ResourcePool::Resource> resource;
  bool resource_is_being_initialized;
  bool contents_swizzled;
  bool need_to_gather_pixel_refs;
  std::list<skia::LazyPixelRef*> pending_pixel_refs;

  // Ephemeral state, valid only during Manage.
  TileManagerBin bin[NUM_TREES];
  // Bin used to determine raster priority.
  TileManagerBin raster_bin;
  TileResolution resolution;
  float time_to_needed_in_seconds;
};

// This class manages tiles, deciding which should get rasterized and which
// should no longer have any memory assigned to them. Tile objects are "owned"
// by layers; they automatically register with the manager when they are
// created, and unregister from the manager when they are deleted.
class CC_EXPORT TileManager {
 public:
  TileManager(TileManagerClient* client,
              ResourceProvider *resource_provider,
              size_t num_raster_threads);
  virtual ~TileManager();

  const GlobalStateThatImpactsTilePriority& GlobalState() const { return global_state_; }
  void SetGlobalState(const GlobalStateThatImpactsTilePriority& state);

  void ManageTiles();
  void CheckForCompletedSetPixels();

  void GetRenderingStats(RenderingStats* stats);

  int GetTilesInBinCount(TileManagerBin bin, WhichTree tree);
  int GetDrawableTilesInBinCount(TileManagerBin bin, WhichTree tree);

 protected:
  // Methods called by Tile
  friend class Tile;
  void RegisterTile(Tile*);
  void UnregisterTile(Tile*);
  void WillModifyTilePriority(Tile*, WhichTree, const TilePriority& new_priority);

 private:
  void ResetBinCounts();
  void AssignGpuMemoryToTiles();
  void FreeResourcesForTile(Tile*);
  void ScheduleManageTiles();
  void ScheduleCheckForCompletedSetPixels();
  void DispatchMoreTasks();
  void DispatchOneRasterTask(RasterThread*, scoped_refptr<Tile>);
  void OnRasterTaskCompleted(
      scoped_refptr<Tile>,
      scoped_ptr<ResourcePool::Resource>,
      scoped_refptr<PicturePileImpl>,
      int manage_tiles_call_count_when_dispatched,
      RenderingStats*);
  void DidFinishTileInitialization(Tile*);
  void DispatchImageDecodingTasksForTile(Tile*);
  void OnImageDecodingTaskCompleted(scoped_refptr<Tile>,
                                    uint32_t,
                                    RenderingStats*);
  void DispatchOneImageDecodingTask(
      RasterThread*, scoped_refptr<Tile>, skia::LazyPixelRef*);
  void GatherPixelRefsForTile(Tile*);
  RasterThread* GetFreeRasterThread();

  TileManagerClient* client_;
  scoped_ptr<ResourcePool> resource_pool_;
  bool manage_tiles_pending_;
  int manage_tiles_call_count_;
  bool check_for_completed_set_pixels_pending_;

  GlobalStateThatImpactsTilePriority global_state_;

  int tiles_in_bin_count_[NUM_BINS][NUM_TREES];
  int drawable_tiles_in_bin_count_[NUM_BINS][NUM_TREES];

  typedef std::vector<Tile*> TileVector;
  TileVector tiles_;
  TileVector tiles_that_need_to_be_rasterized_;

  typedef std::list<Tile*> TileList;
  // Tiles with image decoding tasks. These tiles need to be rasterized
  // when all the image decoding tasks finish.
  TileList tiles_with_image_decoding_tasks_;

  typedef base::hash_map<uint32_t, skia::LazyPixelRef*> PixelRefMap;
  PixelRefMap pending_decode_tasks_;

  typedef std::queue<scoped_refptr<Tile> > TileQueue;
  TileQueue tiles_with_pending_set_pixels_;

  typedef ScopedPtrVector<RasterThread> RasterThreadVector;
  RasterThreadVector raster_threads_;

  RenderingStats rendering_stats_;

  DISALLOW_COPY_AND_ASSIGN(TileManager);
};

}  // namespace cc

#endif  // CC_TILE_MANAGER_H_
