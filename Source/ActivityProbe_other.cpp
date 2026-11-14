// Détection d'activité — autres systèmes (Linux…) : non prise en charge.
// Seule la lecture du transport fait avancer le chrono.
#include "ActivityProbe.h"

struct ActivityProbe::Impl {};

ActivityProbe::ActivityProbe() : impl (std::make_unique<Impl>()) {}
ActivityProbe::~ActivityProbe() = default;

bool ActivityProbe::poll()                    { return false; }
bool ActivityProbe::isHostForeground() const  { return false; }
