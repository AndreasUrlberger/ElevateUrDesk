# Adjust AccelStepper

Add new public function to AccelStepper.hpp
```
void fixMissingSteps(long missedSteps);
```

Add implementation of function to AccelStepper.cpp

```
void AccelStepper::fixMissingSteps(long missedSteps)
{
    // Adjust current position for missed steps depending on direction. Missed steps are always positive.
    if (_direction == DIRECTION_CW)
    {
        _currentPos -= missedSteps;
    }
    else if (_direction == DIRECTION_CCW)
    {
        _currentPos += missedSteps;
    }
    _n -= missedSteps;
}
```

