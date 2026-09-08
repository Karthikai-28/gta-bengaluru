# Traffic System

## Bengaluru-specific objective

Traffic should feel like a system, not decoration.

## Road graph

Each lane contains:

```text
lane_id
speed_limit
vehicle_classes
next_lanes
turn_type
priority
signal_group
width
```

## Driver personality

- Patient
- Normal
- Aggressive
- Cautious
- Commercial
- Auto driver
- Bus driver
- Delivery rider

## Behavior modules

- Car following
- Gap acceptance
- Lane change
- Overtake
- Junction negotiation
- U-turn
- Pull-over
- Parking
- Obstacle avoidance
- Emergency yielding

## Signature behaviors

### Motorcycles
- Accept narrower gaps
- Filter at low speed
- Higher lane-change frequency

### Autos
- Frequent stopping
- Flexible lane positioning
- Higher U-turn frequency

### Buses
- Large braking distance
- Stop at designated points
- Wider turn envelope

## Traffic jams

Jams should arise from actual local blockage and demand, but simulation must use caps to avoid CPU runaway.

## Time-of-day density

- Early morning: light
- Morning commute: heavy
- Midday: moderate
- Evening commute: very heavy
- Late night: light, higher average speed
