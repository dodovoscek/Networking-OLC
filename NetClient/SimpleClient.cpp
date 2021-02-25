#include <iostream>
#include <olc_net.h>

enum class CustomMsgClass : uint32_t
{
	FireBullet,
	MovePlayer
};

int main()
{
	olc::net::message<CustomMsgClass> msg;
	msg.header.id = CustomMsgClass::FireBullet;

	int a = 1;
	bool b = true;
	float c = 3.14F;

	struct
	{
		float x;
		float y;
	} d[5];

	msg << a << b << c << d;

	a = 99;
	b = false;
	c = 3.45F;

	msg >> d >> c >> b >> a;

	return 0;
}