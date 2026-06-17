"""
Canonical dimensions aligned with the UE5 PCG module grid.

All values in centimeters. These match the existing UE5 PCG system constants
so that Blender-generated greybox meshes are dimensionally compatible with
PCG-spawned meshes when both are used in the same level.
"""

import math

# ---------------------------------------------------------------------------
# Module grid (from PCG_SYSTEM_COHESION doc + baroque_mesh_catalog.json)
# ---------------------------------------------------------------------------
WALL_WIDTH = 400          # cm — standard wall module width
STORY_HEIGHT = 600        # cm — one story height
COLUMN_SPACING = 400      # cm — center-to-center column spacing
GOLDEN_RATIO = 0.6180339887498949  # φ⁻¹

# Vertical offsets
CORNICE_OFFSET_Z = 580    # cm — cornice placement height
BALCONY_OFFSET_XY = 120   # cm — balcony outward offset

# Walkability constraints (from FOUNDATION §5)
MAX_STEP_HEIGHT = 50      # cm — maximum navigable step
MIN_PATH_WIDTH = 200      # cm — minimum walkable path width

# ---------------------------------------------------------------------------
# Column defaults
# ---------------------------------------------------------------------------
COLUMN_SHAFT_HEIGHT = STORY_HEIGHT  # 600 cm
COLUMN_RADIUS_BOTTOM = 30           # cm
COLUMN_RADIUS_TOP = 25              # cm (slight taper)
COLUMN_FLUTING_COUNT = 20           # default number of flutes
COLUMN_FLUTING_DEPTH = 3.0          # cm
COLUMN_ENTASIS = 0.02               # subtle swelling ratio
COLUMN_BASE_HEIGHT = 30             # cm
COLUMN_CAPITAL_HEIGHT = 60          # cm

# ---------------------------------------------------------------------------
# Balustrade defaults
# ---------------------------------------------------------------------------
BALUSTRADE_HEIGHT = 100   # cm
BALUSTER_SPACING = 20     # cm
RAIL_HEIGHT = 10          # cm
RAIL_DEPTH = 15           # cm
NEWEL_WIDTH = 25          # cm

# ---------------------------------------------------------------------------
# Molding defaults
# ---------------------------------------------------------------------------
MOLDING_DEPTH = 15        # cm
MOLDING_HEIGHT = 10       # cm

# ---------------------------------------------------------------------------
# Vault defaults
# ---------------------------------------------------------------------------
VAULT_RIB_WIDTH = 12      # cm
VAULT_RIB_DEPTH = 20      # cm
VAULT_THICKNESS = 15      # cm

# ---------------------------------------------------------------------------
# Facade defaults
# ---------------------------------------------------------------------------
FACADE_STORY_HEIGHT = STORY_HEIGHT  # 600 cm
FACADE_WALL_THICKNESS = 30          # cm
WINDOW_WIDTH = 160                  # cm
WINDOW_HEIGHT = 280                 # cm
WINDOW_SILL_HEIGHT = 120            # cm

# ---------------------------------------------------------------------------
# Escher defaults
# ---------------------------------------------------------------------------
ESCHER_STEP_HEIGHT = 20   # cm
ESCHER_STAIR_WIDTH = 120  # cm
ESCHER_STAIR_DEPTH = 30   # cm
ESCHER_STEPS_PER_FLIGHT = 12

# ---------------------------------------------------------------------------
# Mathematical constants
# ---------------------------------------------------------------------------
PHI = (1.0 + math.sqrt(5.0)) / 2.0  # Golden ratio φ ≈ 1.618
TAU = math.pi * 2.0
DEG_TO_RAD = math.pi / 180.0
RAD_TO_DEG = 180.0 / math.pi
