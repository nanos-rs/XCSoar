#ifndef XCSOAR_RASTERTILE_HPP
#define XCSOAR_RASTERTILE_HPP

#include "Terrain/RasterBuffer.hpp"
#include "Geo/GeoBounds.hpp"
#include "Util/NonCopyable.hpp"
#include "Util/ActiveList.hpp"
#include "Util/StaticArray.hpp"

#include <assert.h>
#include <tchar.h>
#include <stddef.h>
#include <stdio.h>

class RasterTile : private NonCopyable {
  struct MetaData {
    unsigned int xstart, ystart, xend, yend;
  };

  unsigned int xstart, ystart, xend, yend;
  unsigned int width, height;
  bool request;

  RasterBuffer buffer;

public:
  RasterTile()
    :xstart(0), ystart(0), xend(0), yend(0),
     width(0), height(0) {}
  ~RasterTile() {
    Disable();
  }

  void set(unsigned _xstart, unsigned _ystart,
           unsigned _xend, unsigned _yend) {
    xstart = _xstart;
    ystart = _ystart;
    xend = _xend;
    yend = _yend;
    width = xend - xstart;
    height = yend - ystart;
  }

  bool defined() const {
    return width > 0 && height > 0;
  }

  bool is_requested() const {
    return request;
  }

  void clear_requested() {
    request = false;
  }

  bool SaveCache(FILE *file) const;
  bool LoadCache(FILE *file);

  bool CheckTileVisibility(const int view_x, const int view_y);

  void Disable() {
    buffer.reset();
  }

  void Enable();
  bool IsEnabled() const {
    return buffer.defined();
  }
  bool IsDisabled() const {
    return !buffer.defined();
  }

  gcc_pure
  short GetField(unsigned x, unsigned y) const;

  gcc_pure
  short GetFieldInterpolated(unsigned x, unsigned y,
                             unsigned ix, unsigned iy) const;

  inline short* GetImageBuffer() {
    return buffer.get_data();
  }

  bool VisibilityChanged(int view_x, int view_y);
};

class RasterTileCache : private NonCopyable {
  static const unsigned MAX_RTC_TILES = 4096;

  /**
   * The maximum number of tiles which are loaded at a time.  This
   * must be limited because the amount of memory is finite.
   */
#if !defined(_WIN32_WCE) && !defined(ANDROID)
  // desktop: use a lot of memory
  static const unsigned MAX_ACTIVE_TILES = 64;
#elif !defined(_WIN32_WCE) || _WIN32_WCE >= 0x0400
  // embedded: use less memory
  static const unsigned MAX_ACTIVE_TILES = 32;
#else
  // old Windows CE: use only little memory
  static const unsigned MAX_ACTIVE_TILES = 16;
#endif

  static const unsigned RTC_SUBSAMPLING = 16;

  struct MarkerSegmentInfo {
    MarkerSegmentInfo() {}
    MarkerSegmentInfo(long _file_offset, int _tile=-1)
      :file_offset(_file_offset), tile(_tile) {}

    /**
     * The position of this marker segment within the file.
     */
    long file_offset;

    /**
     * The associated tile number.  -1 if this segment does not belong
     * to a tile.
     */
    int tile;
  };

  struct CacheHeader {
    enum {
#ifdef FIXED_MATH
      VERSION = 0x2,
#else
      VERSION = 0x3,
#endif
    };

    unsigned version, width, height, num_marker_segments;
    GeoBounds bounds;
  };

  StaticArray<MarkerSegmentInfo, 8192> segments;

public:
  RasterTileCache() {
    Reset();
  }

private:
  bool initialised;

  /** is the "bounds" attribute valid? */
  bool bounds_initialised;

  bool dirty;

  RasterTile tiles[MAX_RTC_TILES];
  mutable ActiveList<const RasterTile, MAX_ACTIVE_TILES> ActiveTiles;
  RasterBuffer Overview;
  bool scan_overview;
  unsigned int width, height;
  GeoBounds bounds;

public:
  gcc_pure
  short GetField(unsigned x, unsigned y) const;

  gcc_pure
  short GetFieldInterpolated(unsigned int lx,
                             unsigned int ly) const;

protected:
  void LoadJPG2000(const char *path);

  /**
   * Load a world file (*.tfw or *.j2w).
   */
  bool LoadWorldFile(const TCHAR *path);

public:
  bool LoadOverview(const char *path, const TCHAR *world_file);

  bool SaveCache(FILE *file) const;
  bool LoadCache(FILE *file);

  void UpdateTiles(const char *path, int x, int y);

  /**
   * Determines if there are still tiles scheduled to be loaded.  Call
   * this after UpdateTiles() to determine if UpdateTiles() should be
   * called again soon.
   */
  bool IsDirty() const {
    return dirty;
  }

  bool GetInitialised() const {
    return initialised;
  }

  void Reset();

  const GeoBounds &GetBounds() const {
    assert(bounds_initialised);

    return bounds;
  }

private:
  gcc_pure
  const MarkerSegmentInfo *
  FindMarkerSegment(long file_offset) const;

public:
  /* callback methods for libjasper (via jas_rtc.cpp) */

  long SkipMarkerSegment(long file_offset) const;
  void MarkerSegment(long file_offset, unsigned id);

  bool TileRequest(unsigned index);

  short *GetOverview() {
    return Overview.get_data();
  }

  void SetSize(unsigned width, unsigned height);
  short* GetImageBuffer(unsigned index);
  void SetLatLonBounds(double lon_min, double lon_max,
                       double lat_min, double lat_max);
  void SetTile(unsigned index, int xstart, int ystart, int xend, int yend);

  void SetInitialised(bool val) {
    initialised = val;
  }

protected:
  bool PollTiles(int x, int y);

public:
  short GetMaxElevation() const {
    return Overview.get_max();
  }

  unsigned int GetWidth() const { return width; }
  unsigned int GetHeight() const { return height; }

private:
  unsigned int overview_width_fine, overview_height_fine;
};

#endif
