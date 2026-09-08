# Procedural Bengaluru World Generation

## Input layers

- Roads
- Road classification
- Building footprints
- Land-use polygons
- Parks
- Water bodies
- Rail/metro alignment where legally usable
- Elevation if available

## Preprocessing

Build a Python tool that outputs normalized game-world JSON/GeoJSON:

```text
source GIS
→ validate geometry
→ convert CRS
→ simplify geometry
→ remove tiny artifacts
→ classify roads
→ infer lane counts when missing
→ classify buildings
→ assign procedural seed
→ export engine-ready records
```

## Road generator

Generate:

- Asphalt mesh
- Lane markings
- Curbs
- Sidewalks
- Medians
- Bus bays
- Auto stands
- Parking slots
- Drain covers
- Speed breakers
- Junction markings

## Building generator

Use modular facade grammar:

- Small independent house
- Apartment
- Mixed-use shop-house
- Office block
- Tech campus
- Industrial shed
- Mall
- Parking structure
- Construction shell

Each building definition should contain:

```text
footprint
height
floor_count
style
facade_palette
roof_type
ground_floor_type
balcony_probability
ac_probability
water_tank_probability
signage_probability
```

## Local detail library

- Compound walls
- Security booths
- Water tanks
- Rooftop dishes
- AC units
- Electrical boxes
- Cable bundles
- Street-food carts
- Small shop shutters
- Apartment gates
- Metro/flyover pillars
- Bus shelters
- Auto stands
- Pothole/decal set
- Construction barricades

## Hero locations

Procedural generation should stop at hero locations. Those areas are rebuilt manually for quality and mission design.
