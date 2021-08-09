# ARSLab_TPS
Temporary repository for an implementation of the Tethered Particle System in the DEVS simulator, Cadmium

=== Messages ===

--- message_t ---

Used to transport impulses (typically only when sent from the random impulse module) or velocities.
Members:
- data: impulse or velocity
- particle_ids: vector of particles the data applies to
- purpose: informs on the purpose of the message (ex. "load", "rest", "ri")

--- tracker_message_t ---

Used to label standard message_t messages when being sent from the responder to the detector.
Members:
- subV_ids: the sub-volumes that the message is relevant to

--- collision_message_t ---

Used to inform the responder of collisions between particles.
Members:
- positions: the IDs and positions of the particles that are colliding (should always have exactly two elements)

--- logging_message_t ---

Used to log the velocities and positions of particles that have been updated.
Members:
- subV_id: the sub-volume from which the message originates
- particle_id: the particle that the velocity and position belongs to
- vecolity: the particle's velocity
- position: the particle's position