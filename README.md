# OpenMPD Framework

OpenMPD is a low-level presentation engine allowing structured exploitation of any combination of MPD content (i.e., visual, tactile, audio) while dealing with the challenges specific to MPD content presentation.

For instance, our multi-rate runtime cycle allows us to combine very high computation rates for the sound-field (i.e., 10K sound-fields per second, for optimum acoustic control), with lower rates for control and rendering processes (i.e., hundreds of Hz, as typically used by rendering engines or tracking devices supporting interaction). Our low-level synchronization allows us to retain accurate interoperation among these processes, All of these enable novel capabilities, such as enabling colour projection onto high-speed particles or swept displays, as well as dexterous manipulations, such as in PoV content changing shape or precise combinations with other PoV paths. 

We first describe the abstractions allowing definition of multimodal content for OpenMPD, and then proceed to describe the stages within the OpenMPD presentation engine, characterizing their performance and detailing key algorithms for data marshalling, acoustic and visual rendering, and synchronization. Then, we illustrate how OpenMPD can be integrated into higher-level tools (i.e., Unity game engine), as a way to facilitate content creation and stimulate MPD adoption. Finally, we use this platform to showcase the presentation capabilities of OpenMPD, such as in allowing combinations of multiple MPD primitives, dexterous manipulation of PoV paths (e.g., high speed particles morphing into different shapes or merging with other particles, to create PoV contents jointly), as well as novel swept-volume MPD content.  


