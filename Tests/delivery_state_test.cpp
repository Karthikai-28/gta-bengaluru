#include "../Source/NammaCity/NammaDeliveryState.h"
#include <cassert>

int main()
{
    FNammaDeliveryState State;
    assert(State.Stage == ENammaDeliveryStage::AwaitingPickup);
    assert(!State.Deliver());  // Cannot complete without the parcel.
    for (int Round = 0; Round < 100; ++Round)
    {
        assert(State.PickUp());
        assert(!State.PickUp());  // No duplicate pickups.
        assert(State.Deliver());
        assert(State.Stage == ENammaDeliveryStage::Complete);
        assert(!State.Deliver());
        assert(!State.PickUp());
        State.Reset();
    }
    assert(State.PickUp());
    State.Reset();  // Restart while carrying drops the parcel.
    assert(!State.Deliver());
    assert(State.PickUp());
    return 0;
}
