#include <iostream>
#include <vector>
#include "track/race_config.h"
#include "track/track_data.h"

int main() {
    std::cout << "Step 1: Creating TrackData..." << std::endl;
    p0::track::TrackData track;
    track.track_id = "test";
    track.length_m = 1000.0;
    std::cout << "Step 1: Done." << std::endl;

    std::cout << "Step 2: Creating RaceDefinition..." << std::endl;
    p0::race::RaceDefinition race;
    std::cout << "Step 2: Done." << std::endl;

    std::cout << "Step 3: Creating RaceManager..." << std::endl;
    p0::track::RaceManager mgr(track, race);
    std::cout << "Step 3: Done." << std::endl;

    std::cout << "Step 4: Creating assignments..." << std::endl;
    std::vector<p0::race::CarAssignment> assignments;
    std::vector<p0::race::TeamDefinition> teams;
    std::cout << "Step 4: Done." << std::endl;

    std::cout << "Step 5: Calling initialize..." << std::endl;
    bool ok = mgr.initialize(assignments, teams);
    std::cout << "Step 5: Done. Result: " << (ok ? "true" : "false") << std::endl;

    return 0;
}
