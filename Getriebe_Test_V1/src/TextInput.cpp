#include "TextInput.hpp"

char TextInput::receivedChars[numChars];

int TextInput::recvWithEndMarker()
{
  byte ndx = 0;
  char endMarker = '\n';
  char rc;
  int dataNumber = 0;
  boolean lineFinished = false;

  while (!lineFinished)
  { // Read each character of the input until a newline appears
    while (Serial.available() == 0)
    {
    } // Wait till more input is available
    rc = Serial.read();
    if (rc != endMarker)
    {
      receivedChars[ndx] = rc;
      ndx++;
      if (ndx >= numChars)
      {
        ndx = numChars - 1;
      }
    }
    else
    {
      receivedChars[ndx] = '\0'; // terminate the string
      dataNumber = atoi(receivedChars);
      lineFinished = true;
    }
  }
  return dataNumber;
}

int TextInput::getIntInput()
{
  int value = 0;
  do
  {
    value = recvWithEndMarker();
  } while (value == 0);
  return value;
}