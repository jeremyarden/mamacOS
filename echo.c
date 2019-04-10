int main()
{
    char argC;
    char *argV;
    interrupt(0x21,0x22,argC);
    if(argC >= 1)
    {
        interrupt(0x21,0x23,0,argV);
        interrupt(0x21, 0x00,argV,0);
    }
    interrupt(0x21, 0x07, 0,0);
	return 0;
}
