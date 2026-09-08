#include "NammaDeliveryState.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNammaDeliveryRulesTest, "NammaCity.Delivery.Rules",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FNammaDeliveryRulesTest::RunTest(const FString& Parameters)
{
    FNammaDeliveryState State;
    TestFalse(TEXT("Delivery without pickup rejected"), State.Deliver());
    TestTrue(TEXT("Pickup accepted"), State.PickUp());
    TestFalse(TEXT("Duplicate pickup rejected"), State.PickUp());
    TestTrue(TEXT("Delivery accepted"), State.Deliver());
    TestFalse(TEXT("Duplicate delivery rejected"), State.Deliver());
    State.Reset();
    TestTrue(TEXT("Replay pickup accepted"), State.PickUp());
    State.Reset();
    TestFalse(TEXT("Restart removes carried parcel"), State.Deliver());
    return true;
}
#endif
