"""
Constants for Melodia Art Nouveau Greybox.

All dimensions in centimeters to match UE5 PCG grid.
Art Nouveau defaults: slimmer columns, organic proportions, whiplash curves.
"""

import math

# ---------------------------------------------------------------------------
# UE5 PCG grid alignment (shared with baroque addon)
# ---------------------------------------------------------------------------
WALL_WIDTH = 400
STORY_HEIGHT = 600
COLUMN_SPACING = 400

# ---------------------------------------------------------------------------
# Art Nouveau column defaults
# ---------------------------------------------------------------------------
COLUMN_SHAFT_HEIGHT = 600
COLUMN_RADIUS_BOTTOM = 25  # Slimmer than baroque
COLUMN_RADIUS_TOP = 18
COLUMN_BASE_RADIUS = 30
COLUMN_CAPITAL_HEIGHT = 60

# ---------------------------------------------------------------------------
# Branching / organic defaults
# ---------------------------------------------------------------------------
STEM_BRANCH_COUNT = 3
STEM_BRANCH_ANGLE = 35  # degrees from vertical
STEM_NODE_BULGE = 0.15  # radius swell at nodes
STEM_DEFAULT_NODES = 3

# ---------------------------------------------------------------------------
# Whiplash curve defaults
# ---------------------------------------------------------------------------
WHIPLASH_AMPLITUDE = 40
WHIPLASH_WAVELENGTH = 200
WHIPLASH_SEGMENTS = 32

# ---------------------------------------------------------------------------
# Wall / facade defaults
# ---------------------------------------------------------------------------
FACADE_WALL_THICKNESS = 25  # Thinner than baroque
ARCH_DEFAULT_WIDTH = 180
ARCH_DEFAULT_HEIGHT = 320
ARCH_DEFAULT_DEPTH = 30

# ---------------------------------------------------------------------------
# Railing defaults
# ---------------------------------------------------------------------------
RAILING_HEIGHT = 100
RAILING_BALUSTER_SPACING = 25

# ---------------------------------------------------------------------------
# Vault defaults
# ---------------------------------------------------------------------------
VAULT_THICKNESS = 12

# ---------------------------------------------------------------------------
# Surface / tile defaults
# ---------------------------------------------------------------------------
MOSAIC_TILE_SIZE = 20
STAINED_GLASS_DEPTH = 3

# ---------------------------------------------------------------------------
# Asymmetry
# ---------------------------------------------------------------------------
MAX_ASYMMETRY = 0.3  # default max deviation ratio (0-1)

# ---------------------------------------------------------------------------
# Math constants
# ---------------------------------------------------------------------------
PHI = (1 + math.sqrt(5)) / 2  # golden ratio ~1.618
TAU = 2 * math.pi
DEG_TO_RAD = math.pi / 180.0
RAD_TO_DEG = 180.0 / math.pi

# ---------------------------------------------------------------------------
# FBX export defaults
# ---------------------------------------------------------------------------
FBX_SCALE = 0.01  # Blender units (cm) to UE5 (m)
