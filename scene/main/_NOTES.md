Q: Is there a more graceful way of handling C callbacks into a C++ member functions?
A: SORT OF -- By making GGPONetwork a singleton, I can use static methods for the callbacks and access GGPO via singleton accessors.

Q: Can this design be adjusted so that project-specific code is relegated to GDScript?
    Is there a more graceful way of serializing our gamestate so that individual nodes can save their important data and restore it on demand?
A: Yes, but with some catches.
Firstly, buttons must be translated to a "GGPOInput" fixed sized struct so that inputs can be buffered and saved. Additionally, all-important gamestate entries must be saved to a Dictionary so that it can be serialized and deserialized. 

Q: Where should input handling happen?
A: TBA -- Should be passed down GDScript probably

Q: What are our priorities? Is it generality? Specific game implementation? Commercial plugin? Learning? "Is it possible" mentality?
A: TBA

Q: Can this design be encapsulated into a Godot module? What needs to happen to get there?
A: TBA

Q: Is it worth using GGPO over custom lockstep / rollback implementation?
A: Yes...?
Lockstep implementation is difficult enough that reusing a tried and true library would be the best way to get results quicker.

Q: How much of Godot network layer is useful in a p2p context?
A: TBA

