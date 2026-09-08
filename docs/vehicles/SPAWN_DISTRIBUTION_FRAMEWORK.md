# Vehicle Spawn and Traffic Distribution Framework

The catalog must **not** spawn uniformly. Bengaluru traffic should vary with district, road class, time, weekday, weather, service routes and local wealth/activity.

## First playable district — initial traffic mix

| Class | Day share target |
|---|---:|
| Scooters + commuter motorcycles | 38% |
| Autos / three-wheelers | 10% |
| Hatchbacks / compact sedans | 17% |
| Compact + mid-size SUVs | 13% |
| Taxis / corporate cabs | 7% |
| EV cars | 4% |
| Vans / delivery / mini trucks | 5% |
| Buses | 3% |
| Heavy trucks / tankers | 2% |
| Luxury / sports / rare | 1% |

These are **gameplay starting targets**, not measured Bengaluru traffic statistics. Replace or retune them after field observations/data collection.

## Required spawn fields

Each production vehicle data asset should eventually have:

- district weights
- road-class eligibility
- weekday/weekend curve
- time-of-day curve
- weather modifier
- wealth-zone modifier
- parked/moving ratio
- service/fleet routes
- rarity tier
- color weights
- trim weights
- age/condition weights
- driver-archetype weights
- cargo/passenger state
- police/mission restrictions
- despawn distance and simulation fidelity tier
