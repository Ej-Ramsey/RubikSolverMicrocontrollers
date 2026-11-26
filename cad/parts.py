"""
parts.py - reference CAD models of the Rubik's Solver parts that have
complete manufacturer drawings:

    * NEMA 17 stepper motor, STEPPERONLINE 17HE15-1504S
    * Raspberry Pi Pico W

These are "reference" models: drop them into your frame assembly to design
mounts and check clearances. They are not meant to be printed.

Run from the project root:

    cad/.venv/bin/python cad/parts.py

It writes STEP (for Fusion 360 / Onshape / SolidWorks / FreeCAD) and STL
files into cad/models/.

All dimensions are in millimetres and come from cad_dimensions.txt. Values
marked APPROX are not on the manufacturer drawing: they are cosmetic or
clearance envelopes and should be checked with calipers.
"""

from pathlib import Path

from build123d import (
    Align,
    Axis,
    Box,
    Cylinder,
    Location,
    Locations,
    Mode,
    Plane,
    Pos,
    chamfer,
    export_step,
    export_stl,
    fillet,
)

# ============================================================================
# Output
# ============================================================================
OUTPUT_DIR = Path(__file__).resolve().parent / "models"  # Where STEP/STL files are written

# ============================================================================
# NEMA 17 motor - STEPPERONLINE 17HE15-1504S (oyostepper drawing, 6.7.2022)
#
# Coordinate system: origin at the centre of the front (mounting) face of
# the flange. The shaft points along +Z; the body extends along -Z.
# The connector is on the -Y side.
# ============================================================================
MOTOR_FLANGE = 42.3             # Square flange size (drawing: 42.3 MAX)
MOTOR_BODY_LENGTH = 38.0        # Flange face to back of the motor (drawing: 38 REF)
MOTOR_CORNER_CHAMFER = 4.0      # APPROX: 45-degree chamfer on the four long body edges (cosmetic)

MOTOR_HOLE_SPACING = 31.0       # M3 hole pattern, centre to centre (drawing: 31 +/- 0.2)
MOTOR_HOLE_DIAMETER = 2.5       # M3 tap drill size, used to show the threaded holes
MOTOR_HOLE_DEPTH = 4.0          # Usable thread depth (drawing: 4.0 MIN)

MOTOR_BOSS_DIAMETER = 22.0      # Raised pilot ring around the shaft (drawing: 22 -0.05)
MOTOR_BOSS_HEIGHT = 2.0         # Pilot height above the flange face (drawing: 2 +/- 0.25)

MOTOR_SHAFT_DIAMETER = 5.0      # Shaft diameter (drawing: 5 -0.012)
MOTOR_SHAFT_LENGTH = 23.5       # Flange face to shaft tip (drawing: 23.5 +/- 0.5)
MOTOR_DCUT_LENGTH = 20.0        # Length of the flat, measured from the tip (drawing: 20 +/- 0.2)
MOTOR_DCUT_ACROSS = 4.5         # Distance across the flat (drawing: 4.5 +/- 0.1)

MOTOR_CONN_WIDTH = 16.0         # Connector width along X (drawing: (16), reference)
MOTOR_CONN_DEPTH = 10.0         # Connector length along Z, from the back face (drawing: (10), reference)
MOTOR_CONN_PROTRUSION = 6.5     # How far the connector sticks out below the body (drawing: (6.5), reference)

# ============================================================================
# Raspberry Pi Pico W (Raspberry Pi product brief, April 2024)
#
# Coordinate system: origin at the centre of the BOTTOM face of the PCB.
# The long side runs along Y, with the micro-USB end at +Y.
# Components are on the top (+Z) side.
# ============================================================================
PICO_LENGTH = 51.0              # Board length (drawing: 51)
PICO_WIDTH = 21.0               # Board width (drawing: 21)
PICO_PCB_THICKNESS = 1.0        # APPROX: PCB thickness, not given on the drawing

PICO_HOLE_DIAMETER = 2.1        # Mounting holes (drawing: 2.1)
PICO_HOLE_SPACING_X = 11.4      # Across the board, centre to centre (drawing: 11.4)
PICO_HOLE_FROM_END = 2.0        # Hole centre to the short edge (drawing: 2)

PICO_PIN_COUNT_PER_ROW = 20     # Pins down each long side
PICO_PIN_PITCH = 2.54           # Pin spacing (drawing: 2.54)
PICO_PIN_ROW_SPACING = 17.78    # Distance between the two pin rows (drawing: 17.78)
PICO_PIN_HOLE_DIAMETER = 1.0    # Pin hole diameter (drawing: 1)

PICO_USB_WIDTH = 8.0            # Micro-USB connector width (drawing: 8)
PICO_USB_OVERHANG = 1.3         # How far the connector sticks out past the board end (drawing: 1.3 typ)
PICO_USB_LENGTH = 5.6           # APPROX: connector length along Y, including the overhang
PICO_USB_HEIGHT = 2.6           # APPROX: connector height above the PCB

# ============================================================================
# Motor mount plate (PRINTABLE)
#
# A square plate that one motor screws onto. The motor's boss sits in the
# centre hole, which centres the shaft. The plate's four corner holes attach
# it to the rest of the frame (rails, standoffs or the neighbouring plates).
#
# Coordinate system: origin at the centre of the BACK face of the plate,
# where the motor's flange face sits. The plate extends along +Z, toward
# the cube, exactly like the motor's shaft. So the motor model and the plate
# model line up with no moving needed.
#
# Print flat, with the back (motor) face on the bed. No supports needed.
# ============================================================================
MOUNT_SIZE = 60.0               # Plate width and height
MOUNT_THICKNESS = 6.0           # Plate thickness
MOUNT_CORNER_RADIUS = 4.0       # Rounding on the plate's four corners

MOUNT_CENTER_HOLE = 22.4        # Fits over the 22 mm boss (0.4 print clearance); a coupler up to ~22 mm passes through
MOUNT_SCREW_CLEARANCE = 3.6     # M3 clearance hole for the motor screws (3.4 printed slightly tight)
MOUNT_COUNTERBORE_DIAMETER = 6.2  # Counterbore for M3 socket-head screws (head is 5.5)
MOUNT_COUNTERBORE_DEPTH = 3.2   # Head height 3.0 + 0.2, so heads sit below the cube-side face
                                # -> M3 x 6 screws: 2.8 through the plate + 3.2 into the motor (4.0 max)

MOUNT_FRAME_HOLE_SPACING = 50.0 # Corner holes for attaching the plate to the frame, centre to centre
MOUNT_FRAME_HOLE = 3.6          # M3 clearance (3.4 printed tight); outside the motor's 42.3 flange so they can be reached


# ============================================================================
# Motor
# ============================================================================
def make_motor():
    """Returns the NEMA 17 motor as one solid."""
    # Body: a square block behind the flange face, with chamfered long edges.
    body = Box(MOTOR_FLANGE, MOTOR_FLANGE, MOTOR_BODY_LENGTH,
               align=(Align.CENTER, Align.CENTER, Align.MAX))
    body = chamfer(body.edges().filter_by(Axis.Z), MOTOR_CORNER_CHAMFER)

    # Four threaded M3 holes in the front face (shown as tap-drill holes).
    half = MOTOR_HOLE_SPACING / 2
    with Locations(*[(x, y, 0) for x in (-half, half) for y in (-half, half)]):
        holes = Cylinder(MOTOR_HOLE_DIAMETER / 2, MOTOR_HOLE_DEPTH,
                         align=(Align.CENTER, Align.CENTER, Align.MAX))
    body = body - holes

    # Pilot boss on the front face.
    boss = Cylinder(MOTOR_BOSS_DIAMETER / 2, MOTOR_BOSS_HEIGHT,
                    align=(Align.CENTER, Align.CENTER, Align.MIN))

    # Shaft from the flange face to the tip, with a D-shaped flat on +X.
    shaft = Cylinder(MOTOR_SHAFT_DIAMETER / 2, MOTOR_SHAFT_LENGTH,
                     align=(Align.CENTER, Align.CENTER, Align.MIN))
    flat_offset = MOTOR_DCUT_ACROSS - MOTOR_SHAFT_DIAMETER / 2   # Flat plane distance from the axis
    flat_cut = Pos(flat_offset, 0, MOTOR_SHAFT_LENGTH - MOTOR_DCUT_LENGTH) * Box(
        MOTOR_SHAFT_DIAMETER, MOTOR_SHAFT_DIAMETER * 2, MOTOR_DCUT_LENGTH + 1,
        align=(Align.MIN, Align.CENTER, Align.MIN))
    shaft = shaft - flat_cut

    # Connector housing at the back of the -Y side, sticking out below the body.
    conn_height = MOTOR_CONN_PROTRUSION + 2       # Overlaps 2 mm into the body so the union is one solid
    connector = Pos(0, -MOTOR_FLANGE / 2 + 2, -MOTOR_BODY_LENGTH) * Box(
        MOTOR_CONN_WIDTH, conn_height, MOTOR_CONN_DEPTH,
        align=(Align.CENTER, Align.MAX, Align.MIN))

    motor = body + boss + shaft + connector
    motor.label = "NEMA17_17HE15-1504S"
    return motor


# ============================================================================
# Pico W
# ============================================================================
def make_pico_w():
    """Returns the Pico W board with its holes and USB connector as one solid."""
    board = Box(PICO_WIDTH, PICO_LENGTH, PICO_PCB_THICKNESS,
                align=(Align.CENTER, Align.CENTER, Align.MIN))

    # Four mounting holes.
    hole_x = PICO_HOLE_SPACING_X / 2
    hole_y = PICO_LENGTH / 2 - PICO_HOLE_FROM_END
    with Locations(*[(x, y, 0) for x in (-hole_x, hole_x) for y in (-hole_y, hole_y)]):
        mount_holes = Cylinder(PICO_HOLE_DIAMETER / 2, PICO_PCB_THICKNESS,
                               align=(Align.CENTER, Align.CENTER, Align.MIN))

    # 2 x 20 pin holes, centred along the board.
    row_x = PICO_PIN_ROW_SPACING / 2
    first_y = (PICO_PIN_COUNT_PER_ROW - 1) * PICO_PIN_PITCH / 2
    pin_positions = [(x, first_y - i * PICO_PIN_PITCH, 0)
                     for x in (-row_x, row_x)
                     for i in range(PICO_PIN_COUNT_PER_ROW)]
    with Locations(*pin_positions):
        pin_holes = Cylinder(PICO_PIN_HOLE_DIAMETER / 2, PICO_PCB_THICKNESS,
                             align=(Align.CENTER, Align.CENTER, Align.MIN))

    board = board - mount_holes - pin_holes

    # Micro-USB connector at the +Y end, overhanging the edge.
    usb = Pos(0, PICO_LENGTH / 2 + PICO_USB_OVERHANG, PICO_PCB_THICKNESS) * Box(
        PICO_USB_WIDTH, PICO_USB_LENGTH, PICO_USB_HEIGHT,
        align=(Align.CENTER, Align.MAX, Align.MIN))

    pico = board + usb
    pico.label = "Raspberry_Pi_Pico_W"
    return pico


# ============================================================================
# Motor mount plate
# ============================================================================
def make_motor_mount():
    """Returns the printable motor mount plate."""
    plate = Box(MOUNT_SIZE, MOUNT_SIZE, MOUNT_THICKNESS,
                align=(Align.CENTER, Align.CENTER, Align.MIN))
    plate = fillet(plate.edges().filter_by(Axis.Z), MOUNT_CORNER_RADIUS)

    # Centre hole: locates the motor's boss and lets the shaft and coupler through.
    plate = plate - Cylinder(MOUNT_CENTER_HOLE / 2, MOUNT_THICKNESS,
                             align=(Align.CENTER, Align.CENTER, Align.MIN))

    # Motor screws: clearance holes, counterbored from the cube side (+Z).
    half = MOTOR_HOLE_SPACING / 2
    motor_holes = [(x, y, 0) for x in (-half, half) for y in (-half, half)]
    with Locations(*motor_holes):
        clearance = Cylinder(MOUNT_SCREW_CLEARANCE / 2, MOUNT_THICKNESS,
                             align=(Align.CENTER, Align.CENTER, Align.MIN))
    with Locations(*[(x, y, MOUNT_THICKNESS) for x, y, _ in motor_holes]):
        counterbore = Cylinder(MOUNT_COUNTERBORE_DIAMETER / 2, MOUNT_COUNTERBORE_DEPTH,
                               align=(Align.CENTER, Align.CENTER, Align.MAX))
    plate = plate - clearance - counterbore

    # Corner holes for fixing the plate to the frame.
    frame_half = MOUNT_FRAME_HOLE_SPACING / 2
    with Locations(*[(x, y, 0) for x in (-frame_half, frame_half)
                     for y in (-frame_half, frame_half)]):
        frame_holes = Cylinder(MOUNT_FRAME_HOLE / 2, MOUNT_THICKNESS,
                               align=(Align.CENTER, Align.CENTER, Align.MIN))
    plate = plate - frame_holes

    plate.label = "Motor_Mount_Plate"
    return plate


# ============================================================================
# Export
# ============================================================================
PRINT_DIR = Path(__file__).resolve().parent / "print"   # Where printable parts are written


def export(part, name, folder=OUTPUT_DIR):
    """Writes `part` to <folder>/<name>.step and .stl."""
    folder.mkdir(parents=True, exist_ok=True)
    export_step(part, str(folder / f"{name}.step"))
    export_stl(part, str(folder / f"{name}.stl"), tolerance=0.01, angular_tolerance=0.1)
    box = part.bounding_box()
    print(f"{name}: {box.size.X:.2f} x {box.size.Y:.2f} x {box.size.Z:.2f} mm, "
          f"volume {part.volume:.0f} mm^3, solids {len(part.solids())}")


if __name__ == "__main__":
    export(make_motor(), "nema17_17HE15-1504S")
    export(make_pico_w(), "pico_w")
    export(make_motor_mount(), "motor_mount_plate", PRINT_DIR)
