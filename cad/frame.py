"""
frame.py - the printed frame for the whole Rubik's Solver.

    open frame  : a skeleton of 12 thin edge rails around the cube (a top
                  ring, a bottom ring and 4 corner columns). Each motor bolts
                  to a small "bridge" (a plate with two arms) that spans
                  between two rails, so most of every cube face is open and
                  the cube can be watched from almost any angle.
                  The top bridge (U motor) lifts off to load the cube.
    base        : a 200 x 200 mm box the frame stands on. It holds the D
                  motor and the electronics (proto board with the Pico W and
                  the drivers), with a stand for the TFT at the front.

Run from the project root:

    cad/.venv/bin/python cad/frame.py

Writes one STL + STEP per printable part to cad/print/frame/, the whole
system as one STEP to cad/models/system_assembly.step, and prints a
collision check.

WORLD COORDINATES (used for the assembly):
    origin = centre of the cube
    +Z up (U face), -Y toward the viewer (F face), +X to the right (R face)

Constants marked MEASURE depend on parts whose size isn't known yet (see
cad_dimensions.txt). They are placeholders: measure the real part and
update them before printing the part that uses them.
"""

from pathlib import Path

from build123d import (
    Align,
    Axis,
    Box,
    Compound,
    Cylinder,
    Location,
    Locations,
    Plane,
    Pos,
    Vector,
    export_step,
    export_stl,
    fillet,
)

import couplers
from couplers import (
    CAP_SEAT_DEPTH,
    CAP_SEAT_HALF,
    CAP_SEAT_RADIUS,
    NUT_DIAMETER,
    PARK_BOSS_DIAMETER,
    PARK_BOSS_HEIGHT,
    PARK_SCREW_PILOT_DEPTH,
    PARKED_BACK_S,
    RECESS_DEPTH,
    RECESS_HALF,
    RECESS_RADIUS,
    make_fixed_coupler,
    make_park_screw_heads,
    make_spring_coupler,
    make_test_fit,
    park_boss_positions,
    rounded_square,
    spring_lengths,
)
from parts import (
    MOTOR_FLANGE,
    MOTOR_CONN_PROTRUSION,
    MOTOR_HOLE_SPACING,
    MOTOR_SHAFT_LENGTH,
    MOUNT_CENTER_HOLE,
    MOUNT_COUNTERBORE_DEPTH,
    MOUNT_COUNTERBORE_DIAMETER,
    MOUNT_SCREW_CLEARANCE,
    MOUNT_THICKNESS,
    make_motor,
)

# ============================================================================
# Output
# ============================================================================
CAD_DIR = Path(__file__).resolve().parent
PRINT_DIR = CAD_DIR / "print" / "frame"     # Printable parts (STL + STEP)
MODEL_DIR = CAD_DIR / "models"              # Full assembly (STEP + STL)

# ============================================================================
# Cube
# ============================================================================
CUBE_SIZE = 55.5                # MoYu RS3M V5 edge length
CUBE_HALF = CUBE_SIZE / 2       # Cube centre to a face (27.75)
LAYER_SWEEP_RADIUS = CUBE_HALF * 2 ** 0.5  # A turning layer's corners sweep this radius (39.2)

# ============================================================================
# Frame size
#
# PANEL_INNER is the most important number: the distance from the cube's
# centre to the inside face of every motor bridge. Bigger = more room to see
# the cube between the motors and the cube, but a longer shaft coupler.
#   shaft tip   = PANEL_INNER + PANEL_THICKNESS - 23.5 = 46.5 from the centre
#   coupler gap = PANEL_INNER - 27.75 = 36.25 from the bridge to the cube face
# Change it and rerun: every part and the checks follow.
# ============================================================================
PANEL_INNER = 64.0              # Cube centre to the inside face of each motor bridge
PANEL_THICKNESS = MOUNT_THICKNESS   # 6: bridge thickness, so M3 x 6 motor screws fit
PANEL_OUTER = PANEL_INNER + PANEL_THICKNESS  # Cube centre to the outside of a bridge (70)

RAIL = 12.0                     # Square size of every rail and column
RAIL_OUTER = PANEL_INNER        # Rails sit just inside the bridges (outer face at 64)
RAIL_INNER = RAIL_OUTER - RAIL  # ... and reach in to 52 (a turning layer only reaches 39.2)
RAIL_CENTER = RAIL_OUTER - RAIL / 2   # Rail centreline (58)

# ============================================================================
# Motor bridges
# ============================================================================
BRIDGE_PLATE = 46.0             # Square plate the motor bolts to (motor flange is 42.3)
BRIDGE_PLATE_RADIUS = 5.0       # Plate corner rounding
BRIDGE_ARM_WIDTH = 16.0         # Width of the two arms that reach out to the rails
BRIDGE_HOLE_OFFSET = 4.0        # Arm-end holes sit at +/- this from the arm centreline

# ============================================================================
# Screws
# ============================================================================
SCREW_CLEARANCE = 3.4           # M3 clearance hole
SCREW_PILOT = 2.5               # M3 pilot for screwing straight into plastic (use 4.0 for heat-set inserts)
PILOT_DEPTH = 8.0               # Depth of pilot holes in rails
COLUMN_PILOT_DEPTH = 10.0       # Depth of the pilot holes in the column ends
BASE_SCREW_PILOT_DEPTH = 10.0   # Depth of the pilots for the base screws (bottom ring)

TOP_PIN_DIAMETER = 4.0          # Locating pins on the top ring for the lift-off top bridge
TOP_PIN_HEIGHT = 4.0            # Shorter than the bridge thickness so the bridge sits flat
TOP_PIN_CLEARANCE = 4.4         # Matching holes in the top bridge

# ============================================================================
# Electronics base
# ============================================================================
BASE_SIZE = 200.0               # Square footprint (fits a 220 mm bed)
BASE_HEIGHT = 48.0              # Top surface to bottom edge of the shell
BASE_WALL = 3.0                 # Side wall thickness
BASE_TOP = 4.0                  # Top plate thickness
BASE_TOP_Z = -PANEL_OUTER       # The bottom ring sits on the base top (z = -70)
BASE_BOTTOM_PLATE = 3.0         # Removable bottom plate thickness

BASE_BOSS_DIAMETER = 9.0        # Corner bosses for the bottom plate screws
BASE_BOSS_CENTER = BASE_SIZE / 2 - 8.0   # Boss centre distance from the axis (92)

D_MOTOR_CLEARANCE = 1.2         # Gap around the D motor in the base top
BASE_SCREW_SPOTS = [(0, RAIL_CENTER), (0, -RAIL_CENTER), (RAIL_CENTER, 0), (-RAIL_CENTER, 0)]
                                # Screws up through the base top into the bottom ring

CABLE_HOLE_DIAMETER = 14.0      # Holes in the base top for the side motors' cables
CABLE_HOLE_CENTER = 90.0        # Distance from the centre (motors overhang the base slightly)

PROTO_BOARD_X = 70.0            # Proto board (7 x 9 cm) size along X
PROTO_BOARD_Y = 90.0            # ... and along Y
PROTO_BOARD_CENTER_X = 61.0     # Board centre (right side of the base, clear of the D motor)
PROTO_HOLE_INSET = 2.5          # MEASURE: corner hole centre to board edge
PROTO_STANDOFF_HEIGHT = 6.0     # Standoff height above the bottom plate
PROTO_STANDOFF_DIAMETER = 6.0   # Standoff outer diameter

VENT_SLOT_WIDTH = 3.0           # Vent slots in the right wall, beside the drivers
VENT_SLOT_HEIGHT = 22.0
VENT_SLOT_COUNT = 8
VENT_SLOT_PITCH = 9.0

IR_HOLE_DIAMETER = 7.0          # MEASURE: front wall hole for the IR receiver's lens
IR_HOLE_X = -70.0               # Position along the front wall
POWER_NOTCH_WIDTH = 12.0        # Notch in the back wall for the 12 V cable
POWER_NOTCH_HEIGHT = 10.0
POWER_NOTCH_X = -70.0

# ============================================================================
# TFT stand (front-right corner of the base top, in front of the frame)
# ============================================================================
TFT_BOARD_WIDTH = 34.0          # MEASURE: TFT board width (portrait)
TFT_BOARD_HEIGHT = 56.0         # MEASURE: TFT board height
TFT_HOLE_SPACING_X = 29.0       # MEASURE: TFT mounting holes, centre to centre
TFT_HOLE_SPACING_Y = 50.0       # MEASURE
TFT_HOLE_PILOT = 2.0            # MEASURE: pilot for M2.5 screws (or match your TFT's holes)
TFT_STAND_CENTER_X = 78.0       # Stand position on the base top
TFT_STAND_FRONT_Y = -97.0       # Front face of the upright, where the TFT sits
TFT_STAND_THICKNESS = 4.0       # Upright thickness
TFT_STAND_LIFT = 6.0            # Gap between the base top and the bottom of the TFT
TFT_FOOT_DEPTH = 12.0           # Foot that screws to the base top
TFT_FOOT_THICKNESS = 5.0
TFT_FOOT_SCREW_X = 12.0         # Foot screws at +/- this from the stand centre
TFT_WIRE_SLOT = (18.0, 6.0)     # Slot in the base top behind the stand for the TFT wires


# ============================================================================
# Helpers
# ============================================================================
def rounded_box(x, y, z, radius, align=(Align.CENTER, Align.CENTER, Align.MIN)):
    """Box with its four vertical edges rounded."""
    return fillet(Box(x, y, z, align=align).edges().filter_by(Axis.Z), radius)


def holes(positions, diameter, depth, align_z=Align.MIN):
    """Vertical cylinders (to add or subtract) at each (x, y, z) position."""
    with Locations(*positions):
        return Cylinder(diameter / 2, depth, align=(Align.CENTER, Align.CENTER, align_z))


def along(point, direction, diameter, depth):
    """A cylinder starting at `point` and running `depth` along `direction`."""
    z_dir = Vector(*direction).normalized()
    helper = Vector(1, 0, 0) if abs(z_dir.X) < 0.9 else Vector(0, 1, 0)
    x_dir = z_dir.cross(helper)              # Any direction square to the cylinder's axis
    loc = Location(Plane(origin=Vector(*point), x_dir=x_dir, z_dir=z_dir))
    return loc * Cylinder(diameter / 2, depth, align=(Align.CENTER, Align.CENTER, Align.MIN))


def face_location(face):
    """Location that places a bridge (or motor) built in "bridge local"
    coordinates onto a face of the frame.

    Bridge local: z = 0 is the OUTSIDE face (where the motor flange sits) and
    +z points toward the cube, exactly like the motor model. The arms run
    along local Y. Local -Y is where the motor's connector points: down for
    the four side motors, toward the back for U and D.
    """
    inward = {"U": (0, 0, -1), "D": (0, 0, 1), "F": (0, 1, 0),
              "B": (0, -1, 0), "R": (-1, 0, 0), "L": (1, 0, 0)}[face]
    z_dir = Vector(*inward)
    y_dir = Vector(0, -1, 0) if face in "UD" else Vector(0, 0, 1)
    x_dir = y_dir.cross(z_dir)
    return Location(Plane(origin=z_dir * -PANEL_OUTER, x_dir=x_dir, z_dir=z_dir))


def motor_mount_cuts():
    """Centre hole and counterbored motor screws, in bridge-local coordinates."""
    cut = Cylinder(MOUNT_CENTER_HOLE / 2, PANEL_THICKNESS,
                   align=(Align.CENTER, Align.CENTER, Align.MIN))
    half = MOTOR_HOLE_SPACING / 2
    xy = [(x, y) for x in (-half, half) for y in (-half, half)]
    cut += holes([(x, y, 0) for x, y in xy], MOUNT_SCREW_CLEARANCE, PANEL_THICKNESS)
    cut += holes([(x, y, PANEL_THICKNESS) for x, y in xy],
                 MOUNT_COUNTERBORE_DIAMETER, MOUNT_COUNTERBORE_DEPTH, Align.MAX)
    return cut


def arm_end_holes_local():
    """(x, y) of the four arm-end holes in bridge-local coordinates:
    two at each end, centred on the rail they sit on."""
    o = BRIDGE_HOLE_OFFSET
    return [(x, y) for y in (-RAIL_CENTER, RAIL_CENTER) for x in (-o, o)]


def is_pin_hole(x):
    """On the top bridge, the -X hole of each arm end is a pin hole."""
    return x < 0


# ============================================================================
# Motor bridges
# ============================================================================
def make_bridge_local(face):
    """A motor bridge (plate + 2 arms) in bridge-local coordinates."""
    plate = rounded_box(BRIDGE_PLATE, BRIDGE_PLATE, PANEL_THICKNESS, BRIDGE_PLATE_RADIUS)
    arms = rounded_box(BRIDGE_ARM_WIDTH, 2 * RAIL_OUTER, PANEL_THICKNESS, 3.0)
    bridge = plate + arms - motor_mount_cuts()
    if face in "FBLR":
        # Bosses for the two park screws that catch the spring coupler's flange.
        spots = [(x, y, PANEL_THICKNESS) for x, y in park_boss_positions()]
        bridge += holes(spots, PARK_BOSS_DIAMETER, PARK_BOSS_HEIGHT)
        bridge -= holes([(x, y, PANEL_THICKNESS + PARK_BOSS_HEIGHT) for x, y, _ in spots],
                        SCREW_PILOT, PARK_SCREW_PILOT_DEPTH, Align.MAX)
    for x, y in arm_end_holes_local():
        size = TOP_PIN_CLEARANCE if face == "U" and is_pin_hole(x) else SCREW_CLEARANCE
        bridge -= holes([(x, y, 0)], size, PANEL_THICKNESS)
    return bridge


def bridge_hole_points(face):
    """[(local_x, world point on the rail surface, inward direction)] for a bridge."""
    loc = face_location(face)
    inward = (loc * Pos(0, 0, 1)).position - (loc * Pos(0, 0, 0)).position
    points = []
    for x, y in arm_end_holes_local():
        p = (loc * Pos(x, y, PANEL_THICKNESS)).position
        points.append((x, (p.X, p.Y, p.Z), (inward.X, inward.Y, inward.Z)))
    return points


# ============================================================================
# Skeleton: rings and columns
# ============================================================================
def square_ring(outer, inner, z0, height):
    """Square ring (outer and inner half-sizes) from z0 up by `height`."""
    ring = Pos(0, 0, z0) * Box(2 * outer, 2 * outer, height,
                               align=(Align.CENTER, Align.CENTER, Align.MIN))
    return ring - Pos(0, 0, z0) * Box(2 * inner, 2 * inner, height,
                                      align=(Align.CENTER, Align.CENTER, Align.MIN))


def column_xy():
    """(x, y) centres of the four corner columns."""
    c = RAIL_CENTER
    return [(x, y) for x in (-c, c) for y in (-c, c)]


def side_bridge_pilots():
    """Pilot holes (into the rails) for the four side bridges' screws."""
    cuts = []
    for face in "FBLR":
        for _, point, inward in bridge_hole_points(face):
            cuts.append(along(point, inward, SCREW_PILOT, PILOT_DEPTH))
    return cuts


def make_top_ring():
    """Top ring of rails, with pins and thumb-screw pilots for the top bridge."""
    ring = square_ring(RAIL_OUTER, RAIL_INNER, RAIL_INNER, RAIL)
    for cut in side_bridge_pilots():
        ring -= cut
    ring -= holes([(x, y, RAIL_INNER) for x, y in column_xy()], SCREW_CLEARANCE, RAIL)
    for x, point, _ in bridge_hole_points("U"):
        if is_pin_hole(x):
            ring += holes([point], TOP_PIN_DIAMETER, TOP_PIN_HEIGHT)
        else:
            ring -= holes([point], SCREW_PILOT, PILOT_DEPTH, Align.MAX)
    ring.label = "top_ring"
    return ring


def make_bottom_ring():
    """Bottom ring: a flat 6 mm frame that sits on the base, the 4 bottom
    rails on top of it, and the D motor's plate and arms in the frame layer."""
    floor_z = -PANEL_OUTER
    frame = square_ring(PANEL_OUTER, RAIL_INNER, floor_z, PANEL_THICKNESS)
    rails = square_ring(RAIL_OUTER, RAIL_INNER, -RAIL_OUTER, RAIL)

    # D motor plate + arms, built like a bridge and placed in the frame layer.
    loc = face_location("D")
    d_plate = loc * (rounded_box(BRIDGE_PLATE, BRIDGE_PLATE, PANEL_THICKNESS, BRIDGE_PLATE_RADIUS)
                     + rounded_box(BRIDGE_ARM_WIDTH, 2 * RAIL_INNER + 4, PANEL_THICKNESS, 3.0))
    ring = frame + rails + d_plate - loc * motor_mount_cuts()

    for cut in side_bridge_pilots():
        ring -= cut
    # Column screws come up from below through the frame and the rail.
    ring -= holes([(x, y, floor_z) for x, y in column_xy()], SCREW_CLEARANCE,
                  PANEL_THICKNESS + RAIL)
    # Pilots for the screws coming up through the base top.
    ring -= holes([(x, y, floor_z) for x, y in BASE_SCREW_SPOTS], SCREW_PILOT,
                  BASE_SCREW_PILOT_DEPTH)
    ring.label = "bottom_ring"
    return ring


def make_column(x, y):
    """One corner column between the rings, with a pilot in each end."""
    col = Pos(x, y, -RAIL_INNER) * Box(RAIL, RAIL, 2 * RAIL_INNER,
                                       align=(Align.CENTER, Align.CENTER, Align.MIN))
    col -= holes([(x, y, -RAIL_INNER)], SCREW_PILOT, COLUMN_PILOT_DEPTH)
    col -= holes([(x, y, RAIL_INNER)], SCREW_PILOT, COLUMN_PILOT_DEPTH, Align.MAX)
    col.label = "column"
    return col


# ============================================================================
# Electronics base
# ============================================================================
def make_base():
    """The base shell (open at the bottom), in world coordinates."""
    bottom_z = BASE_TOP_Z - BASE_HEIGHT
    shell = Pos(0, 0, bottom_z) * rounded_box(BASE_SIZE, BASE_SIZE, BASE_HEIGHT, 6.0)
    inner = BASE_SIZE - 2 * BASE_WALL
    shell -= Pos(0, 0, bottom_z) * rounded_box(inner, inner, BASE_HEIGHT - BASE_TOP, 3.0)

    # Corner bosses for the bottom plate screws.
    c = BASE_BOSS_CENTER
    corners = [(x, y) for x in (-c, c) for y in (-c, c)]
    shell += holes([(x, y, bottom_z) for x, y in corners], BASE_BOSS_DIAMETER,
                   BASE_HEIGHT - BASE_TOP)
    shell -= holes([(x, y, bottom_z) for x, y in corners], SCREW_PILOT, 12.0)

    # Opening for the D motor (its connector points +Y).
    hole_half = MOTOR_FLANGE / 2 + D_MOTOR_CLEARANCE
    hole_back = MOTOR_FLANGE / 2 + MOTOR_CONN_PROTRUSION + D_MOTOR_CLEARANCE
    shell -= Pos(0, (hole_back - hole_half) / 2, BASE_TOP_Z) * Box(
        2 * hole_half, hole_half + hole_back, BASE_TOP,
        align=(Align.CENTER, Align.CENTER, Align.MAX))

    # Screws up into the bottom ring.
    shell -= holes([(x, y, BASE_TOP_Z) for x, y in BASE_SCREW_SPOTS],
                   SCREW_CLEARANCE, BASE_TOP, Align.MAX)

    # Cable holes for the four side motors.
    r = CABLE_HOLE_CENTER
    shell -= holes([(0, -r, BASE_TOP_Z), (0, r, BASE_TOP_Z), (r, 0, BASE_TOP_Z),
                    (-r, 0, BASE_TOP_Z)], CABLE_HOLE_DIAMETER, BASE_TOP, Align.MAX)

    # TFT: wire slot behind the stand, and holes for the stand's foot screws.
    slot_x, slot_y = TFT_WIRE_SLOT
    shell -= Pos(TFT_STAND_CENTER_X, TFT_STAND_FRONT_Y + TFT_FOOT_DEPTH + slot_y,
                 BASE_TOP_Z) * Box(slot_x, slot_y, BASE_TOP,
                                   align=(Align.CENTER, Align.CENTER, Align.MAX))
    foot_y = TFT_STAND_FRONT_Y + TFT_FOOT_DEPTH / 2
    shell -= holes([(TFT_STAND_CENTER_X - TFT_FOOT_SCREW_X, foot_y, BASE_TOP_Z),
                    (TFT_STAND_CENTER_X + TFT_FOOT_SCREW_X, foot_y, BASE_TOP_Z)],
                   SCREW_CLEARANCE, BASE_TOP, Align.MAX)

    # Vent slots in the right wall, beside the drivers.
    vent_z = bottom_z + (BASE_HEIGHT - BASE_TOP) / 2
    first = -(VENT_SLOT_COUNT - 1) * VENT_SLOT_PITCH / 2
    for i in range(VENT_SLOT_COUNT):
        shell -= Pos(BASE_SIZE / 2, first + i * VENT_SLOT_PITCH, vent_z) * Box(
            2 * BASE_WALL + 2, VENT_SLOT_WIDTH, VENT_SLOT_HEIGHT)

    # IR receiver window (front wall) and power cable notch (back wall).
    shell -= Pos(IR_HOLE_X, -BASE_SIZE / 2, vent_z) * Cylinder(
        IR_HOLE_DIAMETER / 2, 2 * BASE_WALL + 2, rotation=(90, 0, 0))
    shell -= Pos(POWER_NOTCH_X, BASE_SIZE / 2, bottom_z) * Box(
        POWER_NOTCH_WIDTH, 2 * BASE_WALL + 2, POWER_NOTCH_HEIGHT,
        align=(Align.CENTER, Align.CENTER, Align.MIN))

    shell.label = "base_shell"
    return shell


def make_base_bottom():
    """Removable bottom plate with proto board standoffs, in world coordinates."""
    plate_top = BASE_TOP_Z - BASE_HEIGHT
    plate = Pos(0, 0, plate_top) * rounded_box(BASE_SIZE, BASE_SIZE, BASE_BOTTOM_PLATE, 6.0,
                                               align=(Align.CENTER, Align.CENTER, Align.MAX))
    c = BASE_BOSS_CENTER
    plate -= holes([(x, y, plate_top) for x in (-c, c) for y in (-c, c)],
                   SCREW_CLEARANCE, BASE_BOTTOM_PLATE, Align.MAX)

    # Standoffs at the proto board's corner holes.
    hx = PROTO_BOARD_X / 2 - PROTO_HOLE_INSET
    hy = PROTO_BOARD_Y / 2 - PROTO_HOLE_INSET
    spots = [(PROTO_BOARD_CENTER_X + x, y, plate_top) for x in (-hx, hx) for y in (-hy, hy)]
    plate += holes(spots, PROTO_STANDOFF_DIAMETER, PROTO_STANDOFF_HEIGHT)
    plate -= holes(spots, SCREW_PILOT, PROTO_STANDOFF_HEIGHT)
    plate.label = "base_bottom"
    return plate


def make_tft_stand():
    """Upright stand for the TFT at the front-right of the base, in world coordinates."""
    x0, y0, z0 = TFT_STAND_CENTER_X, TFT_STAND_FRONT_Y, BASE_TOP_Z
    upright_height = TFT_STAND_LIFT + TFT_BOARD_HEIGHT
    width = TFT_BOARD_WIDTH + 6
    stand = Pos(x0, y0, z0) * Box(width, TFT_STAND_THICKNESS, upright_height,
                                  align=(Align.CENTER, Align.MIN, Align.MIN))
    stand += Pos(x0, y0, z0) * Box(width, TFT_FOOT_DEPTH, TFT_FOOT_THICKNESS,
                                   align=(Align.CENTER, Align.MIN, Align.MIN))
    foot_y = y0 + TFT_FOOT_DEPTH / 2
    stand -= holes([(x0 - TFT_FOOT_SCREW_X, foot_y, z0), (x0 + TFT_FOOT_SCREW_X, foot_y, z0)],
                   SCREW_PILOT, TFT_FOOT_THICKNESS)
    centre_z = z0 + TFT_STAND_LIFT + TFT_BOARD_HEIGHT / 2
    for dx in (-TFT_HOLE_SPACING_X / 2, TFT_HOLE_SPACING_X / 2):
        for dz in (-TFT_HOLE_SPACING_Y / 2, TFT_HOLE_SPACING_Y / 2):
            stand -= along((x0 + dx, y0, centre_z + dz), (0, 1, 0), TFT_HOLE_PILOT,
                           TFT_STAND_THICKNESS)
    stand.label = "tft_stand"
    return stand


# ============================================================================
# Assembly, checks and export
# ============================================================================
def build_all():
    """Returns ({name: (world part, print location or None)}, motors, cube)."""
    printable = {}
    for face in "UFBLR":
        loc = face_location(face)
        printable[f"bridge_{face}"] = (loc * make_bridge_local(face), loc)
    printable["top_ring"] = (make_top_ring(), None)
    printable["bottom_ring"] = (make_bottom_ring(), None)
    for i, (x, y) in enumerate(column_xy()):
        printable[f"column_{i + 1}"] = (make_column(x, y), None)
    printable["base_shell"] = (make_base(), None)
    printable["base_bottom"] = (make_base_bottom(), None)
    printable["tft_stand"] = (make_tft_stand(), None)

    # Couplers: fixed on U and D, spring-loaded on the four sides (engaged).
    for face in "URFDLB":
        loc = face_location(face)
        if face in "UD":
            printable[f"coupler_{face}"] = (loc * make_fixed_coupler(COUPLER_FACE_S), loc)
        else:
            printable[f"coupler_{face}"] = (loc * make_spring_coupler(COUPLER_FACE_S), loc)

    motors = {face: face_location(face) * make_motor() for face in "URFDLB"}
    return printable, motors, make_cube()


COUPLER_FACE_S = PANEL_INNER - CUBE_HALF   # Bridge inside face to the cube face (36.25)
HARDWARE_FACES = "FBLR"                    # Faces with park screws (spring couplers)


def make_cube():
    """Cube placeholder with the real centre recesses (caps removed) and the
    adjustment nut modelled as a solid keep-out in each recess."""
    cube = Box(CUBE_SIZE, CUBE_SIZE, CUBE_SIZE)
    for face in "URFDLB":
        loc = face_location(face)
        face_z = PANEL_OUTER - CUBE_HALF                 # Cube face in bridge-local z
        rim_z = face_z + CAP_SEAT_DEPTH
        cube -= loc * rounded_square(CAP_SEAT_HALF, CAP_SEAT_RADIUS, face_z - 0.01,
                                     CAP_SEAT_DEPTH + 0.01)
        cube -= loc * rounded_square(RECESS_HALF, RECESS_RADIUS, rim_z - 0.01, RECESS_DEPTH)
        cube += loc * couplers.cyl(NUT_DIAMETER, rim_z, RECESS_DEPTH)   # Nut keep-out
    cube.label = "cube_placeholder"
    return cube


def make_hardware():
    """Park screw heads on the side bridges (not printed; for checks)."""
    return {f"park_heads_{f}": face_location(f) * make_park_screw_heads() for f in HARDWARE_FACES}


def face_layer_sweep(face):
    """Space a face layer sweeps when it turns: a cylinder of radius
    LAYER_SWEEP_RADIUS around the face's axis, one cubie thick."""
    loc = face_location(face)
    face_z = PANEL_OUTER - CUBE_HALF
    return loc * couplers.cyl(2 * LAYER_SWEEP_RADIUS, face_z, CUBE_SIZE / 3)


def overlap(a, b):
    """Volume shared by two solids (0 if they only touch)."""
    if not a.bounding_box().overlaps(b.bounding_box()):
        return 0.0
    common = a & b
    return sum(s.volume for s in common.solids()) if common else 0.0


def check(printable, motors, cube):
    """Prints every pair of solids that overlap, and the cube clearances."""
    solids = {name: part for name, (part, _) in printable.items()}
    solids.update({f"motor_{f}": m for f, m in motors.items()})
    solids.update(make_hardware())
    solids["cube"] = cube
    names = list(solids)
    problems = 0
    for i, a in enumerate(names):
        for b in names[i + 1:]:
            volume = overlap(solids[a], solids[b])
            if volume > 0.01:
                print(f"  OVERLAP {a} / {b}: {volume:.2f} mm^3")
                problems += 1

    # A turning layer sweeps a cylinder of radius LAYER_SWEEP_RADIUS around
    # its axis, within +/- CUBE_HALF along the axis. Frame parts must stay
    # clear of all of it.
    frame_parts = {n: p for n, (p, _) in printable.items() if not n.startswith("coupler")}
    for axis, rot in (("X", (0, 90, 0)), ("Y", (90, 0, 0)), ("Z", (0, 0, 0))):
        sweep = Cylinder(LAYER_SWEEP_RADIUS, CUBE_SIZE, rotation=rot)
        for name, part in frame_parts.items():
            if overlap(sweep, part) > 0.01:
                print(f"  CUBE LAYER ({axis} axis) HITS {name}")
                problems += 1

    # Each face layer turning must miss every coupler except its own, which
    # turns with it.
    for face in "URFDLB":
        sweep = face_layer_sweep(face)
        for other in "URFDLB":
            if other != face and overlap(sweep, printable[f"coupler_{other}"][0]) > 0.01:
                print(f"  {face} LAYER TURNING HITS coupler_{other}")
                problems += 1

    # Spring couplers turning with their face: the flange must miss the park
    # screw heads at every angle (checked every 15 degrees).
    for face in HARDWARE_FACES:
        loc = face_location(face)
        heads = loc * make_park_screw_heads()
        for angle in range(0, 90, 15):
            if overlap(loc * make_spring_coupler(COUPLER_FACE_S, twist=angle), heads) > 0.01:
                print(f"  coupler_{face} flange hits a park screw at {angle} deg")
                problems += 1

    # Parked (pulled back and twisted 45 deg): the plug must be fully out of
    # the cube, and the flange must sit under the screw heads.
    solid_cube = Box(CUBE_SIZE, CUBE_SIZE, CUBE_SIZE)
    for face in HARDWARE_FACES:
        loc = face_location(face)
        parked = loc * make_spring_coupler(COUPLER_FACE_S, back_s=PARKED_BACK_S, twist=couplers.PARK_TWIST)
        if overlap(parked, solid_cube) > 0.01:
            print(f"  parked coupler_{face} still reaches into the cube")
            problems += 1
        for name in (f"bridge_{face}", f"park_heads_{face}"):
            if overlap(parked, solids[name]) > 0.01:
                print(f"  parked coupler_{face} overlaps {name}")
                problems += 1
    tip_gap = COUPLER_FACE_S - (COUPLER_FACE_S + CAP_SEAT_DEPTH + couplers.PLUG_DEPTH
                                - (couplers.SLIDER_BACK_S - PARKED_BACK_S))
    engaged, parked_len, pulled = spring_lengths()
    print(f"  parked plug clears the cube face by {tip_gap:.2f} mm; spring length "
          f"engaged {engaged:.1f} / parked {parked_len:.1f} / pulled {pulled:.1f} mm")

    shaft_tip = PANEL_OUTER - MOTOR_SHAFT_LENGTH
    print(f"  shaft tip {shaft_tip:.2f} mm from centre, {shaft_tip - CUBE_HALF:.2f} mm "
          f"short of the cube face; coupler gap {PANEL_INNER - CUBE_HALF:.2f} mm")
    print(f"  a turning layer sweeps out to {LAYER_SWEEP_RADIUS:.1f} mm; the frame starts "
          f"at {RAIL_INNER:.1f} mm")
    print("  collisions:", problems)
    return problems


def print_orientation(name, part, loc):
    """Moves a part into the orientation it should be printed in."""
    if loc is not None:                       # Bridges flat, motor side down; couplers
        return loc.inverse() * part           # standing on their back end
    if name == "base_shell":                  # Shell: upside down, top surface on the bed
        return Location((0, 0, 0), (180, 0, 0)) * part
    if name == "tft_stand":                   # Stand: lying on its back
        return Location((0, 0, 0), (-90, 0, 0)) * part
    return part                               # Rings flat, columns standing: as built


# Printable files: one per distinct part (identical copies share a file).
EXPORTS = {
    "bridge_top": "bridge_U",
    "bridge_side": "bridge_F",                # F, B, L and R are identical
    "top_ring": "top_ring",
    "bottom_ring": "bottom_ring",
    "column": "column_1",                     # All four are identical
    "base_shell": "base_shell",
    "base_bottom": "base_bottom",
    "tft_stand": "tft_stand",
    "coupler_fixed": "coupler_U",             # U and D are identical
    "coupler_spring": "coupler_F",            # F, B, L and R are identical
}


def export_all(printable, motors, cube):
    PRINT_DIR.mkdir(parents=True, exist_ok=True)
    MODEL_DIR.mkdir(parents=True, exist_ok=True)
    for file, name in EXPORTS.items():
        part, loc = printable[name]
        flat = print_orientation(name, part, loc)
        bb = flat.bounding_box()
        flat = Pos(-bb.center().X, -bb.center().Y, -bb.min.Z) * flat
        export_stl(flat, str(PRINT_DIR / f"{file}.stl"), tolerance=0.01, angular_tolerance=0.1)
        export_step(flat, str(PRINT_DIR / f"{file}.step"))
        size = flat.bounding_box().size
        print(f"  {file:12s} {size.X:6.1f} x {size.Y:6.1f} x {size.Z:6.1f} mm")

    # Quick test piece: just the neck and plug, to try in a real centre piece.
    test = make_test_fit(COUPLER_FACE_S)
    bb = test.bounding_box()
    test = Pos(-bb.center().X, -bb.center().Y, -bb.min.Z) * test
    export_stl(test, str(PRINT_DIR / "coupler_test_fit.stl"), tolerance=0.01, angular_tolerance=0.1)
    export_step(test, str(PRINT_DIR / "coupler_test_fit.step"))

    parts = [p for p, _ in printable.values()] + list(motors.values()) + [cube]
    export_step(Compound(parts), str(MODEL_DIR / "system_assembly.step"))
    export_stl(Compound(parts), str(MODEL_DIR / "system_assembly.stl"),
               tolerance=0.05, angular_tolerance=0.2)


if __name__ == "__main__":
    printable, motors, cube = build_all()
    print("Checks:")
    check(printable, motors, cube)
    print("Print files:")
    export_all(printable, motors, cube)
