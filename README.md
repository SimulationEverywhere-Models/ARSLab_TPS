# ARSLab_TPS
Temporary repository for an implementation of the Tethered Particle System in the DEVS simulator, Cadmium

=== Messages ===

--- message_t ---

Used to transport impulses (typically only when sent from the random impulse module) or velocities.
Members:
- data: impulse or velocity
- particle_ids: vector of particles the data applies to
- positions: used for logging and is only non-empty when the responder sends a loading message (serves no functional purpose within TPS)
- purpose: informs on the purpose of the message (ex. "load", "rest", "ri")

--- tracker_message_t ---

Used to label standard message_t messages when being sent from the responder to the detector.
Members:
- subV_ids: the sub-volumes that the message is relevant to

Note: tracker messages do not carry "positions" from message_t messages since it is a logging tool only needed once

--- collision_message_t ---

Used to inform the responder of collisions between particles.
Members:
- positions: the IDs and positions of the particles that are colliding (should always have exactly two elements)