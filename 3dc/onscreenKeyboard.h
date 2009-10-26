#ifndef _onscreenKeyboard_h_
#define _onscreenKeyboard_h_

extern "C"
{
void Osk_Draw();
bool Osk_IsActive();
void Osk_MoveLeft();
void Osk_MoveRight();
void Osk_MoveUp();
void Osk_MoveDown();
void Osk_Activate();
void Osk_Deactivate();
//char Osk_GetSelectedKeyChar();
char Osk_HandleKeypress();
};

#endif