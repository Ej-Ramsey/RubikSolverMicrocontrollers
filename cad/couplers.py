"""
couplers.py - shaft couplers that link each motor to a cube face.

With the centre cap pulled off, every centre piece has a rounded-square
recess around its screw and adjustment nut. Each coupler has a hollow
rounded-square plug that drops into that recess (so it can't slip) and a
neck that sits in the cap's old seat, pressing on the centre piece's rim.

Two kinds:
    coupler_fixed  (x2, U and D)  clamped to the shaft with an M3 set screw.
    coupler_spring (x4, F B L R)  slides on the shaft's flat. A compression
                   spring pushes it into the recess. To load the cube, pull
                   it back until its flange touches the bridge, twist 45 deg
                   and let go: the flange catches under two button-head
                   screws on the bridge and stays parked. Twist back to
                   release.

All geometry is built in "bridge-local" coordinates (see frame.py): z = 0 is
the outside of the bridge where the motor flange sits, +z points at the cube,
the shaft's flat faces +X. Distances called `s` below are measured from the
INSIDE face of the bridge (local z = PANEL_THICKNESS) toward the cube.

Cube numbers come from the RS3M STL (a 54.04 mm remake), scaled to the real
55.5 mm cube (x 1.027). Those marked MEASURE should be checked with calipers.
The plug and well sizes have been confirmed with printed test fits.
"""

from build123d import Align, Axis, Box, Cylinder, Pos, fillet

from parts import MOTOR_BOSS_HEIGHT, MOTOR_SHAFT_LENGTH, MOUNT_THICKNESS

# ============================================================================
# Cube centre piece, cap removed (measured from the STL, scaled x 1.027)
# ============================================================================
CAP_SEAT_HALF = 9.15            # MEASURE: half-width of the seat the cap's top plate sat in
CAP_SEAT_RADIUS = 5.6           # MEASURE: corner radius of that seat
CAP_SEAT_DEPTH = 1.54           # MEASURE: cube face to the top (rim) of the centre piece
RECESS_DEPTH = 5.8              # MEASURE: straight-walled depth below the rim
NUT_DIAMETER = 12.0             # MEASURE: blue adjustment nut (about 12 mm in the photos)

# ============================================================================
# Plug (the part that goes into the cube)
# ============================================================================
NECK_CLEARANCE = 0.25           # Gap between the neck and the cap seat, per side
PLUG_CLEARANCE = 0.2            # Gap between the plug and the recess walls, per side
NECK_OUTSIDE = 1.0              # Neck starts this far outside the cube face
PLUG_DEPTH = 3.0                # How far the plug goes into the recess, below the rim
# Plug and well sizes below were confirmed with printed test fits in a real
# centre piece (17.0 was one wall line too big; a 13.1 well was barely tight).
PLUG_WIDTH = 16.2               # Outside width of the plug, walls included
PLUG_RADIUS = 3.6               # Corner radius of the plug
WELL_DIAMETER = 13.3            # Hollow in the plug for the screw and blue adjustment nut
WELL_DEPTH = 6.5                # How deep that hollow goes, measured up from the plug tip
# Recess in the cube model only (the plug plus its clearance), used by
# frame.py for the collision check:
RECESS_HALF = PLUG_WIDTH / 2 + PLUG_CLEARANCE    # 8.3 (16.6 across)
RECESS_RADIUS = PLUG_RADIUS + PLUG_CLEARANCE     # 3.8
BODY_DIAMETER = 18.0            # Round body between the shaft and the neck (fits inside the
                                # 18.5 mm centre piece, so turning side layers never touch it)

# ============================================================================
# Shaft bores (the motor shaft is 5 mm with a flat 4.5 mm across)
# ============================================================================
FIXED_BORE = 5.15               # Snug bore for the fixed coupler
FIXED_FLAT = 4.65               # Across the flat, snug (flat sits 0.075 clear of the shaft's)
SLIDE_BORE = 5.3                # Sliding bore for the spring coupler
SLIDE_FLAT = 4.8                # Across the flat, sliding (0.15 clear)
BORE_PAST_SHAFT = 2.0           # Bore continues this far past the shaft tip
SHAFT_TIP_S = MOTOR_SHAFT_LENGTH - MOUNT_THICKNESS   # Shaft tip, measured from the bridge inside face (17.5)
MOTOR_BOSS_S = MOTOR_BOSS_HEIGHT - MOUNT_THICKNESS   # Top of the motor's boss, inside the bridge hole (-4)

# ============================================================================
# Fixed coupler (U and D)
# ============================================================================
FIXED_BACK_S = 3.0              # Back end of the coupler (gap to the bridge)
SET_SCREW_S = 10.0              # Set screw position along the shaft
SET_SCREW_HOLE = 3.6            # M3 set screw clearance, through to the flat (3.4 prints tight)
NUT_TRAP_WIDTH = 5.8            # M3 nut across flats + clearance
NUT_TRAP_THICKNESS = 2.7        # M3 nut thickness + clearance
NUT_TRAP_X = 4.0                # Nut pocket starts this far from the axis (outside the bore)

# ============================================================================
# Spring coupler (F, B, L, R) and its parking catch
# ============================================================================
FLANGE_DIAMETER = 44.0          # Grip flange at the back (fits the 46 mm bridge plate)
FLANGE_THICKNESS = 3.5
PARK_RADIUS = 17.0              # Park screws sit at (+/-17, 0) on the bridge
PARK_BOSS_DIAMETER = 4.6        # Slim printed post under each park screw (fits the arc slot)
PARK_BOSS_HEIGHT = 4.5          # Post height = height of the washer's underside
PARK_WASHER_DIAMETER = 7.0      # M3 washer under the screw head: this is what catches the flange
PARK_WASHER_THICKNESS = 0.5
PARK_HEAD_DIAMETER = 5.7        # M3 x 10 button-head screw head (ISO 7380)
PARK_HEAD_HEIGHT = 1.65
PARK_SCREW_PILOT_DEPTH = 10.0   # Pilot through the post into the bridge (0.5 mm skin left)
FLANGE_NOTCH_WIDTH = 8.0        # Wide part of each keyhole: passes the washer
FLANGE_SLOT_WIDTH = 5.2         # Arc part of each keyhole: passes the post, not the washer
PARK_TWIST = 45.0               # Twist (degrees, counter-clockwise) to park
ROTATION_GAP = 1.0              # Gap between the flange and the screw heads while turning
SLIDER_BACK_S = (PARK_BOSS_HEIGHT + PARK_WASHER_THICKNESS + PARK_HEAD_HEIGHT
                 + ROTATION_GAP)                                  # Flange back when engaged (7.65)
PARKED_BACK_S = PARK_BOSS_HEIGHT - FLANGE_THICKNESS              # Flange back when parked (1.0)
SPRING_POCKET_DIAMETER = 10.0   # Pocket in the back of the slider that centres the spring
SPRING_POCKET_DEPTH = 3.0


# ============================================================================
# Helpers
# ============================================================================
def rounded_square(half, radius, z0, height):
    """Rounded-square prism, centred on the Z axis, from z0 up by `height`."""
    box = Box(2 * half, 2 * half, height, align=(Align.CENTER, Align.CENTER, Align.MIN))
    return Pos(0, 0, z0) * fillet(box.edges().filter_by(Axis.Z), radius)


def cyl(diameter, z0, height):
    """Cylinder on the Z axis from z0 up by `height`."""
    return Pos(0, 0, z0) * Cylinder(diameter / 2, height,
                                    align=(Align.CENTER, Align.CENTER, Align.MIN))


def d_bore(bore, flat, z0, height):
    """D-shaped hole matching the motor shaft (flat on +X)."""
    hole = cyl(bore, z0, height)
    flat_x = flat - bore / 2                # The flat plane, measured from the axis
    return hole - Pos(flat_x, 0, z0) * Box(bore, 2 * bore, height,
                                           align=(Align.MIN, Align.CENTER, Align.MIN))


def local_z(s):
    """Bridge-local z for a distance `s` from the bridge's inside face."""
    return MOUNT_THICKNESS + s


# ============================================================================
# Shared front end: body, neck and plug
# ============================================================================
def plug_end(face_s, back_s,
             plug_half=PLUG_WIDTH / 2,
             plug_radius=PLUG_RADIUS,
             well_depth=WELL_DEPTH,
             well_diameter=WELL_DIAMETER):
    """Round body from back_s to the cube, neck in the cap seat, hollow plug
    in the recess. `face_s` = distance from the bridge inside face to the cube face.
    plug_half / plug_radius: half-width and corner radius of the plug's outside.
    well_depth: how deep the hollow goes, measured up from the plug tip.
    well_diameter: how wide the hollow is.
    All couplers and the test fit use the defaults."""
    rim_s = face_s + CAP_SEAT_DEPTH
    neck_s = face_s - NECK_OUTSIDE
    tip_s = rim_s + PLUG_DEPTH
    body = cyl(BODY_DIAMETER, local_z(back_s), neck_s - back_s + 0.01)
    neck = rounded_square(CAP_SEAT_HALF - NECK_CLEARANCE, CAP_SEAT_RADIUS - NECK_CLEARANCE,
                          local_z(neck_s), rim_s - neck_s)
    plug = rounded_square(plug_half, plug_radius, local_z(rim_s), PLUG_DEPTH)
    part = body + neck + plug
    # Hollow plug: clears the screw and the adjustment nut.
    part -= cyl(well_diameter, local_z(tip_s - well_depth), well_depth)
    return part


# ============================================================================
# Couplers
# ============================================================================
def make_fixed_coupler(face_s):
    """Fixed coupler (U and D), in bridge-local coordinates."""
    part = plug_end(face_s, FIXED_BACK_S)
    bore_len = SHAFT_TIP_S + BORE_PAST_SHAFT - FIXED_BACK_S
    part -= d_bore(FIXED_BORE, FIXED_FLAT, local_z(FIXED_BACK_S), bore_len)

    # M3 set screw along +X onto the shaft's flat, with a nut pocket that
    # opens to the side (+Y) so the nut can be slid in.
    z = local_z(SET_SCREW_S)
    part -= Pos(0, 0, z) * Cylinder(SET_SCREW_HOLE / 2, BODY_DIAMETER, rotation=(0, 90, 0),
                                    align=(Align.CENTER, Align.CENTER, Align.MIN))
    part -= Pos(NUT_TRAP_X, -NUT_TRAP_WIDTH / 2, z - NUT_TRAP_WIDTH / 2) * Box(
        NUT_TRAP_THICKNESS, BODY_DIAMETER, NUT_TRAP_WIDTH,
        align=(Align.MIN, Align.MIN, Align.MIN))
    part.label = "coupler_fixed"
    return part


def make_spring_coupler(face_s, back_s=SLIDER_BACK_S, twist=0.0):
    """Spring coupler (F, B, L, R), in bridge-local coordinates.
    back_s: where its flange back sits (SLIDER_BACK_S = engaged,
    PARKED_BACK_S = parked). twist: rotation about the shaft in degrees."""
    # Built in the engaged position, then moved as a whole.
    part = plug_end(face_s, SLIDER_BACK_S)

    # Grip flange with four bayonet keyholes, one every 90 degrees, so two of
    # them line up with the park screws whatever position the face ended in.
    flange = cyl(FLANGE_DIAMETER, local_z(SLIDER_BACK_S), FLANGE_THICKNESS)
    for angle in (0, 90, 180, 270):
        flange -= keyhole(local_z(SLIDER_BACK_S)).rotate(Axis.Z, angle)
    part += flange

    # Sliding D-bore (long enough for the shaft tip when pulled fully back)
    # and the spring pocket in the back.
    bore_len = SHAFT_TIP_S + BORE_PAST_SHAFT - SLIDER_BACK_S + SLIDER_BACK_S + 0.5
    part -= d_bore(SLIDE_BORE, SLIDE_FLAT, local_z(SLIDER_BACK_S), bore_len)
    part -= cyl(SPRING_POCKET_DIAMETER, local_z(SLIDER_BACK_S), SPRING_POCKET_DEPTH)

    part = Pos(0, 0, back_s - SLIDER_BACK_S) * part
    if twist:
        part = part.rotate(Axis.Z, twist)
    part.label = "coupler_spring"
    return part


def keyhole(z0):
    """One bayonet keyhole through the flange, starting at angle 0 (+X):
    a round hole that passes the washer, then an arc slot running clockwise
    for PARK_TWIST degrees. Twisting the flange counter-clockwise by
    PARK_TWIST brings the slot's far end onto the post, under the washer.
    The flange's outer rim stays whole, so nothing is left hanging."""
    t = FLANGE_THICKNESS
    notch = Pos(PARK_RADIUS, 0, 0) * cyl(FLANGE_NOTCH_WIDTH, z0, t)
    inner = PARK_RADIUS - FLANGE_SLOT_WIDTH / 2
    outer = PARK_RADIUS + FLANGE_SLOT_WIDTH / 2
    ring = cyl(2 * outer, z0, t) - cyl(2 * inner, z0, t)
    big = 2 * FLANGE_DIAMETER
    below_x_axis = Pos(0, 0, z0) * Box(big, big, t, align=(Align.CENTER, Align.MAX, Align.MIN))
    above_end = (Pos(0, 0, z0) * Box(big, big, t, align=(Align.CENTER, Align.MIN, Align.MIN))
                 ).rotate(Axis.Z, -PARK_TWIST)
    arc = ring & below_x_axis & above_end
    end = Pos(PARK_RADIUS, 0, 0) * cyl(FLANGE_SLOT_WIDTH, z0, t)
    return notch + arc + end.rotate(Axis.Z, -PARK_TWIST)


def park_boss_positions():
    """(x, y) of the two park screw bosses on a side bridge (bridge-local)."""
    return [(-PARK_RADIUS, 0.0), (PARK_RADIUS, 0.0)]


def make_park_screw_heads():
    """The two park washers and button heads (hardware, for clearance checks only)."""
    heads = None
    for x, y in park_boss_positions():
        washer = Pos(x, y, 0) * cyl(PARK_WASHER_DIAMETER, local_z(PARK_BOSS_HEIGHT),
                                    PARK_WASHER_THICKNESS)
        head = Pos(x, y, 0) * cyl(PARK_HEAD_DIAMETER,
                                  local_z(PARK_BOSS_HEIGHT + PARK_WASHER_THICKNESS),
                                  PARK_HEAD_HEIGHT)
        both = washer + head
        heads = both if heads is None else heads + both
    return heads


def make_test_fit(face_s):
    """A short slice of the plug end (neck + plug + a thin disc to hold) for
    trying the fit in a real centre piece before printing all six."""
    part = plug_end(face_s, face_s - NECK_OUTSIDE - 3.0)
    part.label = "coupler_test_fit"
    return part


def spring_lengths():
    """Spring length (mm) when engaged, parked and pulled fully back."""
    seat = MOTOR_BOSS_S
    engaged = SLIDER_BACK_S + SPRING_POCKET_DEPTH - seat
    parked = PARKED_BACK_S + SPRING_POCKET_DEPTH - seat
    pulled = 0.0 + SPRING_POCKET_DEPTH - seat
    return engaged, parked, pulled
