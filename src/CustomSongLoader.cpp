#include "SongLoader.hpp"

// Create the C# objects from all of the song info objects/difficulties.
void CreateObjects() {
    // TODO: Iterate over all of the objects and createManual for everything required (potentially make custom types?)
    // This should involve all of the groundwork for these types, whereas the actual linkage of these types to something relevant
    // ex: The collection of songs doesn't need to happen until later.
}

// We would also like a way of performing an object creation step for just a SINGLE song/path type
// This is because we would like to only create objects over the NEW data, as opposed to ALL data (unless we create caches here anyways)
// We want to differentiate between songs that are already loaded and ones that we need to load.
// Perhaps we can do a level ID comparison? but then a song modified on runtime would not be properly cleared...
// Perhaps we simply perform a full reload every time, clearing everything and remaking everything...
// Which is probably fast enough, even for large N