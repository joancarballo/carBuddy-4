#pragma once
// Convierte las lecturas del tactil en toques: uno por cada vez que se apoya el dedo.

namespace Touch {

// Devuelve true una sola vez por toque, al apoyar el dedo, con sus coordenadas
// de pantalla. Hay que llamarla en cada vuelta del loop().
bool pollTap(int& x, int& y);

}  // namespace Touch
