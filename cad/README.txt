CAD REFERENCE MODELS
====================

Reference models of the parts with complete manufacturer drawings. Import
them into your CAD assembly to design the frame, mounts and couplers around
them. They are for fitting and clearance checks, not for printing.

  models/nema17_17HE15-1504S.step / .stl   NEMA 17 stepper motor (x6)
  models/pico_w.step / .stl                Raspberry Pi Pico W
  previews/*.png                           Pictures of the models

Printable parts (in print/):

  print/motor_mount_plate.stl / .step      Motor mount plate (print 6)

Use the STEP files in Fusion 360, Onshape, SolidWorks or FreeCAD. They come
in as real solids, so you can click faces and edges to constrain against.
STL files are meshes, for slicers or programs without STEP support.

Where each model's origin is
----------------------------
  Motor:  centre of the front (mounting) face of the flange.
          Shaft points along +Z, body goes back along -Z.
          The D-flat on the shaft faces +X; the connector is on the -Y side.
  Pico W: centre of the bottom face of the PCB.
          Long side along Y, micro-USB end at +Y, parts on top (+Z).

  Mount:  centre of the back (motor) face of the plate, plate extends
          along +Z. Same frame as the motor, so the two line up as-is.

Motor mount plate
-----------------
  60 x 60 x 6 mm plate. One per motor, six in total.
  - Centre hole 22.4: the motor's 22 mm boss drops into it, which centres
    the shaft. A shaft coupler up to about 22 mm across passes through.
  - 4 motor screw holes on the 31 mm pattern, counterbored on the cube
    side so the screw heads sit below the surface.
    Screws: M3 x 6 socket head (4 per motor, 24 total). Longer screws can
    bottom out in the motor's 4 mm threads.
  - 4 corner holes, 50 mm apart, M3 clearance, for fixing the plate to the
    frame. They sit outside the motor body, so they can be reached.
  - The shaft sticks out 17.5 mm past the cube-side face of the plate.
  Print flat with the motor side on the bed. No supports needed. Use at
  least 4 walls / 40% infill so the plate stays stiff.
  Check with one print first: the motor should drop onto the centre hole
  with no play. If it is tight, raise MOUNT_CENTER_HOLE by 0.1-0.2.

Whole-system frame (frame.py)
=============================

  cad/.venv/bin/python cad/frame.py

Builds every frame part, checks the whole system for collisions (every
part against every other part and every motor, plus the space each cube
layer sweeps when it turns), and writes:

  print/frame/*.stl / .step            Printable parts, already in print
                                       orientation
  models/system_assembly.step / .stl   Everything assembled, with the six
                                       motors and a 55.5 mm placeholder cube
  previews/system_*.png, frame_skeleton.png, bottom_ring.png,
  base_shell_print_orientation.png, visibility_before_closed_cage.png
  (the earlier closed design, for comparison)

Layout: an open frame, so the cube can be watched
-------------------------------------------------
  Skeleton: 12 rails (12 x 12 mm) along the edges of a 128 mm box,
  printed as a top ring, a bottom ring and 4 corner columns.
  Motor bridges: each motor bolts to a "bridge", a 46 mm plate with two
  16 mm arms that reach out and screw onto two opposite rails. Everything
  else on each face is open.
  The frame stands on a 200 x 200 x 48 mm electronics base. The D motor's
  plate is built into the bottom ring and the motor hangs down into the
  base.

    PANEL_INNER = 64   cube centre to the inside of every motor bridge.
                       Larger leaves more room to see the cube between the
                       motors and the cube, but needs a longer coupler.
                       Right now the coupler spans 36.25 mm from the bridge
                       to the cube face, and the shaft reaches the first
                       17.5 mm of that. Change this one number and rerun;
                       every part and the checks follow.

  Overall size: frame 140 mm across (the old closed cage was 112); with
  motors about 216 mm across, so the side motors overhang the 200 mm base
  by about 8 mm. Height about 230 mm from the bottom of the base to the
  top of the U motor.

  Motor connectors: the four side motors' plugs point DOWN (cables drop to
  the 14 mm holes in the base top). U and D point to the BACK.

Print list (220 mm bed, 0.2 mm layers, PLA or PETG)
---------------------------------------------------
  Part          Qty  Size (mm)          How to print
  bridge_side    4   128 x 46 x 10.5    Flat, motor side down (park posts up)
  bridge_top     1   128 x 46 x 6       Flat, motor side down
  top_ring       1   128 x 128 x 16     Flat, pins up
  bottom_ring    1   140 x 140 x 18     Flat, as saved (thin frame on the bed)
  column         4   12 x 12 x 104      Standing up
  base_shell     1   200 x 200 x 48     Upside down (top on the bed)
  base_bottom    1   200 x 200 x 9      Flat, standoffs up
  tft_stand      1   40 x 62 x 12       Lying on its back
  coupler_spring 4   44 x 44 x 33       Standing on the flange
  coupler_fixed  2   18 x 18 x 38       Standing on its back end
  coupler_test_fit 1 18 x 18 x 9        Plug up (print this first!)
  No supports needed anywhere. Use 4 walls and 30-40% infill for the
  bridges, rings and columns so the motors stay rigid.
  Print these two cheap fit tests first:
    motor_mount_plate  - a motor should drop onto its centre hole without
                         wobbling (the bridges use the same hole).
    coupler_test_fit   - with a centre cap popped off, the plug should
                         drop into the recess snugly, the neck should sit
                         in the cap's seat, and it must not touch the
                         blue nut. If not, adjust the MEASURE constants
                         at the top of couplers.py and rerun.
  Couplers: 100% infill or 6 walls; the plug walls are thin.

Screws (all M3 unless noted; the frame's pilot holes are 2.5 mm, for
screwing straight into the plastic. For heat-set inserts change SCREW_PILOT
to 4.0.)
---------------------------------------------------------------------------
  M3 x 6  socket head   24   Motors to bridges / bottom ring (4 per motor)
  M3 x 12               16   Side bridges to the rails (2 per arm end)
  M3 x 20                4   Top ring down into the columns
  M3 x 25                4   Up through the bottom ring into the columns
  M3 x 12 thumb screw    2   Top bridge to the top ring (lift-off top)
  M3 x 12                4   Base top up into the bottom ring
  M3 x 8                 2   TFT stand to the base top, from below
  M3 x 10                4   Bottom plate to the base shell
  M3 x 8                 4   Proto board to its standoffs
  M2.5 x 8               4   TFT to its stand (MEASURE your TFT's holes)
  M3 x 10 button head    8   Park screws on the side bridges (2 per bridge)
  M3 washer (7 mm)       8   Under each park screw head (this is the catch)
  M3 x 6 set screw       2   Fixed couplers (U, D) onto the shaft flat
  M3 nut                 2   In the fixed couplers' side pockets

Springs: 4 compression springs, one per side coupler, sitting on the
shaft between the motor's boss and the coupler.
  inside diameter  >= 5.5 mm (slides over the 5 mm shaft)
  outside diameter <= 9.5 mm (fits the 10 mm pocket in the coupler)
  free length       16-18 mm (it works at 14.7 mm engaged, 8 mm parked)
  solid length     <= 6.5 mm (it must compress to 7 mm)
  wire              0.5-0.6 mm (light: about 0.5 N/mm)

Assembly order
--------------
  1. Screw the 4 columns between the bottom ring (M3 x 25 from below) and
     the top ring (M3 x 20 from above).
  2. Bolt the D motor under the bottom ring, connector to the back. Screw
     heads go in from the top (the counterbores face the cube).
  3. Bolt each side motor to a side bridge (easiest while the bridge is
     loose), then screw the bridges onto the front, back, left and right
     rails. Their arms run vertically.
  4. Stand the frame on the base so the D motor drops through the opening,
     and with the base's bottom plate off, screw up into the bottom ring.
  5. Route the side motors' cables down through the four cable holes.
     Mount the proto board on the bottom plate, the TFT on its stand, and
     close the bottom plate.
  6. Couplers: slide a fixed coupler onto the D and U shafts (nut in its
     side pocket, set screw facing the shaft's flat) until its back end is
     3 mm from the plate (a 3 mm hex key makes a good spacer), and
     tighten. On each side shaft, slide
     on a spring, then a spring coupler. Screw two M3 x 10 button heads
     with washers into each side bridge's park posts.
  7. Bolt the U motor to the top bridge.

Loading the cube (pop all six centre caps off first)
----------------------------------------------------
  Motors must be OFF (drivers disabled) so the shafts turn by hand.
  1. Park each side coupler: pull it back toward its motor until the
     flange stops against the bridge, twist it 45 degrees counter-clockwise
     (as seen from the cube) and let go. The flange catches under the two
     washers and stays back. Any of its 4 keyholes works.
  2. Lift off the top bridge (2 thumb screws), drop the cube onto the D
     coupler so its plug goes into the bottom centre recess.
  3. Un-park each side coupler: twist it back 45 degrees, and the spring
     snaps the plug into the centre recess. If the recess isn't lined up,
     nudge that face until it clicks in.
  4. Put the top bridge back on its pins (wiggle the U coupler into the
     top recess) and tighten the thumb screws.
  To unload: park the four side couplers, lift the top, lift the cube out.

Still to design / measure
-------------------------
  - Couplers: the centre-piece numbers come from the RS3M STL (a 54.04 mm
    remake) scaled to the real 55.5 mm cube. Print coupler_test_fit and
    check it in a real centre piece. Most important: the blue adjustment
    nut must fit inside the plug's 12.6 mm hole (NUT_DIAMETER, PLUG_BORE).
  - Constants marked MEASURE in frame.py: proto board hole inset, TFT size
    and hole spacing, IR receiver hole. Only base_bottom and tft_stand use
    them, so measure before printing those two. Everything else can be
    printed now.

Accuracy
--------
Every dimension comes from cad_dimensions.txt (manufacturer drawings). A few
values are not on the drawings and are marked APPROX in parts.py:
  - motor corner chamfer size (cosmetic only)
  - Pico W PCB thickness (1.0) and the USB connector's length and height
The Pico W's small components (chip, buttons, Wi-Fi shield) are not
modelled; leave about 3 mm of clearance above the board.

Rebuilding or changing the models
---------------------------------
All dimensions are constants at the top of parts.py. Edit one and rerun.

One-time setup (from the project root):
    python3 -m venv cad/.venv
    cad/.venv/bin/pip install -r cad/requirements.txt

Then:
    cad/.venv/bin/python cad/parts.py
