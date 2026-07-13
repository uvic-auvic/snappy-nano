#pragma once
// Default competition mission (RoboSub 2026 "Restore and Recovery").
//
// Shape:
//   - Root Fallback: run the guarded mission; if it fails for ANY reason,
//     EmergencySurface takes over. Fallback has memory in BT.CPP v4, so an
//     aborted mission is never re-entered — the sub cannot re-submerge to
//     re-run a mission it just aborted.
//   - ReactiveSequence re-checks the safety guards every tick and halts the
//     running leaf when one trips (each leaf's onHalted() shuts off whatever
//     it turned on).
//   - Per-task skip Fallbacks: competition is scored per task, so a failed
//     task latches <task>_skipped and the run continues to the next one.
//   - Every leaf that is not self-bounded sits under a Timeout decorator;
//     timeout means the task FAILED and the skip pattern moves on.
//
// Every thrust/duration marked TUNE is a placeholder for pool testing. Test
// on the bench first: run scripts/fake_sensors.py and watch
// `ros2 topic echo /planner/task` — never in the water before the controller
// consumes this contract correctly.
//
// Override at runtime with the `tree_file` ROS parameter to iterate on the
// mission without recompiling.

namespace snappy::bt
{

inline constexpr const char* kMissionXml = R"(
<root BTCPP_format="4" main_tree_to_execute="MainTree">

  <BehaviorTree ID="MainTree">
    <Fallback name="MissionWithAbort">
      <ReactiveSequence name="GuardedMission">
        <SensorsFreshOK max_age_s="1.5"/>
        <DepthEnvelopeOK max_depth_m="4.0"/>
        <MissionTimeLeft/>
        <!-- KillSwitchOK / BatteryOK: add when those signals exist on a topic -->
        <SubTree ID="Mission"/>
      </ReactiveSequence>
      <EmergencySurface max_ascent_s="45" surfaced_depth_m="0.3"/>
    </Fallback>
  </BehaviorTree>

  <BehaviorTree ID="Mission">
    <Sequence name="MissionSequence">

      <!-- Rules require submerging before leaving the start zone, and the
           coin flip means the initial heading is unknown — so dive first,
           search for the gate second. Not skippable: if we cannot even
           submerge, abort the run. -->
      <Timeout msec="30000">
        <SetDepth depth_m="1.0"/>
      </Timeout>

      <Fallback>
        <SubTree ID="GateTask"/>
        <SetBlackboard output_key="gate_skipped" value="true"/>
      </Fallback>

      <!-- The gate->slalom path marker would guide this transition, but the
           down-camera model has no path-marker class yet — dead-reckoned. -->

      <Fallback>
        <SubTree ID="SlalomTask"/>
        <SetBlackboard output_key="slalom_skipped" value="true"/>
      </Fallback>

      <Fallback>
        <SubTree ID="BinsTask"/>
        <SetBlackboard output_key="bins_skipped" value="true"/>
      </Fallback>

      <Fallback>
        <SubTree ID="TorpedoTask"/>
        <SetBlackboard output_key="torpedo_skipped" value="true"/>
      </Fallback>

      <Fallback>
        <SubTree ID="OctagonTask"/>
        <SetBlackboard output_key="octagon_skipped" value="true"/>
      </Fallback>

      <Fallback>
        <SubTree ID="ReturnHome"/>
        <AlwaysSuccess/>
      </Fallback>

      <!-- End the run on the surface either way. A timeout here fails the
           mission and falls through to EmergencySurface — also acceptable. -->
      <Timeout msec="60000">
        <Surface/>
      </Timeout>

    </Sequence>
  </BehaviorTree>

  <!-- Rotate-and-search: 12 x 30 deg = one full revolution. Each turn settles
       before Locate re-checks, or the camera sweeps past the object mid-turn
       and the debounce never accumulates. Weak for the down camera (a yaw
       spin never changes what is underneath) — replace with a lawnmower
       pattern once closed-loop strafing exists. -->
  <BehaviorTree ID="SearchAndAlign">
    <Sequence>
      <RetryUntilSuccessful num_attempts="12">
        <Fallback>
          <Locate camera="{camera}" object="{object}" consecutive="3" window_s="2.0"
                  detection="{found}"/>
          <ForceFailure>
            <Timeout msec="15000">
              <TurnRelative yaw_deg="30" settle_s="0.5"/>
            </Timeout>
          </ForceFailure>
        </Fallback>
      </RetryUntilSuccessful>
      <Timeout msec="30000">
        <AlignToObject camera="{camera}" object="{object}" tolerance="0.08" settle_s="1.0"/>
      </Timeout>
    </Sequence>
  </BehaviorTree>

  <!-- Gate: find our role's divider icon, latch which side it is on (every
       later task reads it), save the approach heading for ReturnHome, then
       pass under with a 360 roll — a continuous roll is the max-style move,
       a 90-and-back wobble scores zero. -->
  <BehaviorTree ID="GateTask">
    <Sequence>
      <Timeout msec="90000">
        <SubTree ID="SearchAndAlign" camera="front" object="role_gate_icon"/>
      </Timeout>
      <LatchGateSide window_s="3.0"/>
      <SaveHeading key="gate"/>
      <DisableTracking/>
      <Surge thrust_pct="35" seconds="5.0"/>   <!-- TUNE: half passage -->
      <Spin axis="roll" rate_dps="45" angle_deg="360"/>
      <Wait seconds="2.0"/>                    <!-- let the stabiliser re-level -->
      <Surge thrust_pct="35" seconds="4.0"/>   <!-- TUNE: clear the gate -->
    </Sequence>
  </BehaviorTree>

  <!-- Slalom: stay on the gate side of the red pipes. With no lateral
       position estimate the side rule rides on the entry alignment and
       straight legs. -->
  <BehaviorTree ID="SlalomTask">
    <Sequence>
      <Timeout msec="60000">
        <SubTree ID="SearchAndAlign" camera="front" object="slalom"/>
      </Timeout>
      <DisableTracking/>
      <Surge thrust_pct="30" seconds="12.0"/>  <!-- TUNE: three pipe sets -->
    </Sequence>
  </BehaviorTree>

  <!-- Bins: centre OVER the role-matching bin with the down camera, drop
       both markers while the servo holds station, then release the servo. -->
  <BehaviorTree ID="BinsTask">
    <Sequence>
      <Timeout msec="60000">
        <SubTree ID="SearchAndAlign" camera="down" object="role_bin_icon"/>
      </Timeout>
      <DropMarker side="left"/>
      <Wait seconds="1.0"/>
      <DropMarker side="right"/>
      <DisableTracking/>
    </Sequence>
  </BehaviorTree>

  <!-- Torpedoes: "circle" is the opening class. Large-then-small sequencing
       needs per-opening classes the model does not have — both torpedoes go
       through the best-aligned opening. Stand-off distance is the
       controller-side tracker's job via distance_m. -->
  <BehaviorTree ID="TorpedoTask">
    <Sequence>
      <Timeout msec="60000">
        <SubTree ID="SearchAndAlign" camera="front" object="circle"/>
      </Timeout>
      <FireTorpedo side="left"/>
      <Wait seconds="2.0"/>
      <FireTorpedo side="right"/>
      <DisableTracking/>
    </Sequence>
  </BehaviorTree>

  <!-- Octagon: no hydrophones this cycle, so getting there is a timed
       dead-reckon from bins. Surfacing INSIDE the ring is a controlled
       mission Surface (not the emergency ascent), the run continues, and we
       re-submerge for ReturnHome. Claw/basket manipulation is a stretch goal
       — not in the default tree. -->
  <BehaviorTree ID="OctagonTask">
    <Sequence>
      <SaveHeading key="pre_octagon"/>
      <Surge thrust_pct="35" seconds="15.0"/>  <!-- TUNE: bins -> octagon leg -->
      <Timeout msec="45000">
        <Surface/>
      </Timeout>
      <Wait seconds="5.0"/>                    <!-- surface window (visual inspection) -->
      <Timeout msec="30000">
        <SetDepth depth_m="1.0"/>              <!-- re-submerge, run continues -->
      </Timeout>
    </Sequence>
  </BehaviorTree>

  <!-- Return home: reciprocal of the saved gate heading, re-acquire the gate
       visually if possible (any icon side counts on the way back), and pass
       through underwater. A failed re-acquire does not abort the leg — the
       dead-reckon fallback must exist. -->
  <BehaviorTree ID="ReturnHome">
    <Sequence>
      <Timeout msec="20000">
        <FaceSavedHeading key="gate" reciprocal="true"/>
      </Timeout>
      <Fallback>
        <Timeout msec="90000">
          <SubTree ID="SearchAndAlign" camera="front" object="gate_icons"/>
        </Timeout>
        <AlwaysSuccess/>
      </Fallback>
      <DisableTracking/>
      <Surge thrust_pct="35" seconds="20.0"/>  <!-- TUNE: back through the gate -->
    </Sequence>
  </BehaviorTree>

</root>
)";

}  // namespace snappy::bt
