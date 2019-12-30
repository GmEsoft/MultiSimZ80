#ifndef IRQ_I_H
#define IRQ_I_H

class IRQ_I
{
public:

	IRQ_I(void)
	{
	}

	virtual ~IRQ_I(void)
	{
	}

	virtual void trigger() = 0;
};

#endif
