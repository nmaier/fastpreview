static int InstructionSet() {
	static int _rv = -1;
	static int cpuinfo[4];
	static unsigned _feat_edx, _feat_ecx;

	if (_rv >= 0) {
		return _rv;
	}
	_rv = 0;
	__cpuid(&cpuinfo, 1);
	_feat_edx = (unsigned)cpuinfo[3];
	_feat_ecx = (unsigned)cpuinfo[2];

#define __btn(a, b) if (!((a >> b) & 1)) goto done
	if (sizeof(void*) == 4) {
		__btn(_feat_edx, 23);
		++_rv; // MMX == 1;
		__btn(_feat_edx, 24);
		__btn(_feat_edx, 25);
		_rv = 3; // FXSAVE/XMM/SSE == 3;
		__btn(_feat_edx, 26);
		++_rv; // SSE2 == 4;
	}
	else {
		_rv = 4; // SSE2;
	}
	__btn(_feat_ecx, 0);
	++_rv; // SSE3 == 5;
	__btn(_feat_ecx, 9);
	++_rv; // SSSE3 == 5;
	__btn(_feat_ecx, 19);
	_rv = 8; // SSE4.1 == 5;
	__btn(_feat_ecx, 20);
	_rv = 10; // SSE4.1 == 5;

#undef __btn
done:
	return _rv;
}

#ifdef __cplusplus 
extern "C" {        // Avoid C++ name mangling 
#endif 

int __intel_cpu_indicator = 0;  
void __intel_cpu_indicator_init() { 

   // Get CPU level from asmlib library function 
   switch (InstructionSet()) { 
   case 0:  // No special instruction set supported 
   default: 
      __intel_cpu_indicator = 1; 
      break; 
   case 1: case 2: // MMX supported 
      __intel_cpu_indicator = 8; 
      break; 
   case 3:  // SSE supported 
      __intel_cpu_indicator = 0x80;
      break; 
   case 4:  // SSE2 supported 
      __intel_cpu_indicator = 0x200; 
      break; 
   case 5:  // SSE3 supported 
      __intel_cpu_indicator = 0x800; 
      break; 
   case 6: case 7:  // Supplementary-SSE3 supported 
      __intel_cpu_indicator = 0x1000; 
      break; 
   case 8: case 9:  // SSE4.1 supported 
      __intel_cpu_indicator = 0x2000; 
      break; 
   case 10: case 11: // SSE4.2 and POPCNT supported 
      __intel_cpu_indicator = 0x2000; 
      break; 
   case 12: // AVX, PCLMUL and AES supported 
      __intel_cpu_indicator = 0x20000; 
      break; 
   } 
}

int mkl_serv_cpu_detect(void) { 
   int n = InstructionSet(); // Get instruction set 
   if (n < 4) return 0; // SSE2 not supported 
 
   if (sizeof(void*) == 4) { 
      // 32 bit mode 
      switch (n) { 
      case 4:  // SSE2 supported 
         n = 2; break; 
      case 5:  // SSE3 supported 
         n = 3; break; 
      case 6: case 7: case 8: case 9: // Supplementary-SSE3 supported 
         n = 4; break; 
      case 10: // SSE4.2 supported 
         n = 5; break; 
      default: // AVX 
         // Set n = 6 if MKL library supports AVX 
         // (currently experimental in 64-bit version of MKL 10.2) 
         n = 5; break;
      } 
   } 
   else { 
      // 64 bit mode 
      switch (n) { 
      case 4:  // SSE2 supported 
         n = 0; break; 
      case 5:  // SSE3 supported 
         n = 1; break; 
      case 6: case 7: case 8: case 9: // Supplementary-SSE3 supported 
         n = 2; break; 
      case 10: // SSE4.2 supported 
         n = 3; break; 
      default: // AVX 
         // Set n = 4 if MKL library supports AVX 
         // (currently experimental in 64-bit version of MKL 10.2) 
         n = 3; break; 
      } 
   } 
   return n;
}

int mkl_vml_service_cpu_detect(void) { 
   int n = InstructionSet(); 
   if (n < 4) return 0; // SSE2 not supported 
 
   if (sizeof(void*) == 4) { 
      // 32 bit mode 
      switch (n) { 
      case 4:  // SSE2 supported 
         n = 2; break; 
      case 5:  // SSE3 supported 
         n = 3; break; 
      case 6: case 7: // Supplementary-SSE3 supported 
         n = 4; break; 
      case 8: case 9: // SSE4.1 supported 
         n = 5; break; 
      case 10:  // SSE4.2 supported 
         n = 6; break; 
      default:  // AVX supported  
         // Set n = 7 if VML library supports AVX 
         // (currently experimental in 64-bit version of MKL 10.2) 
         n = 6; break;
      } 
   } 
   else { 
      // 64 bit mode 
      switch (n) { 
      case 4: case 5: // SSE2 supported 
         n = 1; break; 
      case 6: case 7: // Supplementary-SSE3 supported 
         n = 2; break; 
      case 8: case 9: // SSE4.1 supported 
         n = 3; break; 
      case 10:  // SSE4.2 supported 
         n = 4; break; 
      default:  // AVX supported  
         // Set n = 5 if VML library supports AVX 
         // (currently experimental in 64-bit version of MKL 10.2) 
         n = 4; break; 
      } 
   } 
   return n; 
}

#ifdef __cplusplus 
}        // Avoid C++ name mangling 
#endif 