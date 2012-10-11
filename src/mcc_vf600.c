/* This field contains CPU-to-CPU interrupt vector numbers for all device cores */
static const unsigned int mcc_cpu_to_cpu_vectors[] = { 48, 16 };

unsigned int mcc_get_cpu_to_cpu_vector(unsigned int core)
{
   if (core < (sizeof(mcc_cpu_to_cpu_vectors)/sizeof(mcc_cpu_to_cpu_vectors[0]))) {
      return  mcc_cpu_to_cpu_vectors[core];

   }
   return 0;
}

void mcc_clear_cpu_to_cpu_interrupt(unsigned int core)
{
   if(core == 0) {
       *(unsigned int *)(void*)(0x40001800) = (1 << 0);
   }
   else if(core == 1) {
       *(unsigned int *)(void*)(0x40001804) = (1 << 0);
   }
}

void mcc_triger_cpu_to_cpu_interrupt(void)
{
   *(unsigned int *)(void*)(0x40001820) = (1 << 24);
}
