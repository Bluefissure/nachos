// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "noff.h"
#include "bitmap.h"

static BitMap *pageMap = new BitMap(NumPhysPages);


//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void 
SwapHeader (NoffHeader *noffH)
{
	noffH->noffMagic = WordToHost(noffH->noffMagic);
	noffH->code.size = WordToHost(noffH->code.size);
	noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
	noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
	noffH->initData.size = WordToHost(noffH->initData.size);
	noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
	noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
	noffH->uninitData.size = WordToHost(noffH->uninitData.size);
	noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
	noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//
//	Assumes that the object code file is in NOFF format.
//
//	First, set up the translation from program memory to physical 
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//
//	"executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------

AddrSpace::AddrSpace(OpenFile *executable)
{
    NoffHeader noffH;
    unsigned int i, size, j;

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

// how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
			+ UserStackSize;	// we need to increase the size
						// to leave room for the stack
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;

    ASSERT(numPages <= NumPhysPages);		// check we're not trying
						// to run anything too big --
						// at least until we have
						// virtual memory

    DEBUG('a', "Initializing address space, num pages %d, size %d\n", 
					numPages, size);

	if(numPages > pageMap->NumClear()){
		printf("Not enough pages in pageMap.");
		return;
	}
	ASSERT(numPages <= (unsigned int) pageMap->NumClear());


// first, set up the translation 
    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++) {
		pageTable[i].virtualPage = i;	// for now, virtual page # = phys page #
		pageTable[i].physicalPage = pageMap->Find();	//use pageMap to manage 
		pageTable[i].valid = TRUE;
		pageTable[i].use = FALSE;
		pageTable[i].dirty = FALSE;
		pageTable[i].readOnly = FALSE;  // if the code segment was entirely on 
						// a separate page, we could set its 
						// pages to be read-only
    }
    
// zero out the entire address space, to zero the unitialized data segment 
// and the stack segment
//    bzero(machine->mainMemory, size);		//should not use bzero to set from head
	for (i = 0; i < numPages; i++){
		for (j = 0; j < PageSize; j++)
			machine->mainMemory[ pageTable[i].physicalPage * PageSize + j ] = 0;
	}

	RestoreState();	//store the addrspace in order to translate virtAddr

// then, copy in the code and data segments into memory
    if (noffH.code.size > 0) {
        DEBUG('a', "Initializing code segment, at 0x%x, size %d\n", 
			noffH.code.virtualAddr, noffH.code.size);
		int phyAddr;
		machine->Translate(noffH.code.virtualAddr, &phyAddr, 4, TRUE);
        executable->ReadAt(&(machine->mainMemory[phyAddr]),	//may out of bound
			noffH.code.size, noffH.code.inFileAddr);
    }
    if (noffH.initData.size > 0) {
        DEBUG('a', "Initializing data segment, at 0x%x, size %d\n", 
			noffH.initData.virtualAddr, noffH.initData.size);
		int phyAddr;
		machine->Translate(noffH.initData.virtualAddr, &phyAddr, 4, TRUE);
        executable->ReadAt(&(machine->mainMemory[phyAddr]),	//may out of bound
			noffH.initData.size, noffH.initData.inFileAddr);
    }

}


void AddrSpace::setPreAddrSpace(AddrSpace *preSpace)
{
	preAddrSpace = preSpace;
}
void AddrSpace::setNextAddrSpace(AddrSpace *nextSpace)
{
	nextAddrSpace = nextSpace;
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space and reset the bit map
//
// Implemented by Bluefissure.
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
	unsigned int numRegPages, i;
	for (i = 0; i < numPages; i++){	//reset the bitmap
		pageMap->Clear(pageTable[i].physicalPage);
	}
	delete [] pageTable;
	if(regPageTable != NULL){
		numRegPages = divRoundUp(NumTotalRegs * 4, PageSize);
		for (i = 0; i < numRegPages; i++){	//reset the bitmap
			pageMap->Clear(regPageTable[i].physicalPage);
		}
		delete [] regPageTable;
	}
	delete preAddrSpace;
	delete nextAddrSpace;
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);	

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG('a', "Initializing stack register to %d\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//	numPagesReg -- number of pages to store registers
// Implemented by Bluefissure.
//----------------------------------------------------------------------

void AddrSpace::SaveState() //Save register to memory and save memory
{
	unsigned int numRegPages, i, j, offset;
	numRegPages = divRoundUp(NumTotalRegs * 4, PageSize);
	regPageTable = new TranslationEntry[numRegPages];
    for (i = 0; i < numRegPages; i++) {
		regPageTable[i].virtualPage = i;
		regPageTable[i].physicalPage = pageMap->Find();
		regPageTable[i].valid = TRUE;
		regPageTable[i].use = FALSE;
		regPageTable[i].dirty = FALSE;
		regPageTable[i].readOnly = FALSE;  
	}
    machine->pageTable = regPageTable;
    machine->pageTableSize = numRegPages;
	for (i = 0, j = 0, offset = 0; i < NumTotalRegs; i++, offset += 4){
		if(offset >= PageSize){
			offset %= PageSize;
			j++;
		}
		machine->WriteMem(j * PageSize + offset, 4, machine->registers[i]);
	}
}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//	Restore the registers stored in memery 
//		and tell machine to find current pagetable
// Implemented by Bluefissure.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
	unsigned int numRegPages, i, j, offset;
	if(regPageTable != NULL){
		numRegPages = divRoundUp(NumTotalRegs * 4, PageSize);
		machine->pageTable = regPageTable;
		machine->pageTableSize = numRegPages;
		for (i = 0, j = 0, offset = 0; i < NumTotalRegs; i++, offset += 4){
			if(offset >= PageSize){
				offset %= PageSize;
				j++;
			}
			int tmpRegValue;
			machine->ReadMem(j * PageSize + offset, 4, &tmpRegValue);
			machine->WriteRegister(i, tmpRegValue);
		}

	}
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
}

//----------------------------------------------------------------------
// AddrSpace::Print
//  Page tables dumping 
// Implemented by Bluefissure
//----------------------------------------------------------------------

void AddrSpace::Print() 
{
	printf("page table dump: %d pages in total\n", numPages);
	printf("============================================\n");
	printf("\tVirtPage, \tPhysPage\n");
	for (int i=0; i < numPages; i++) {
		printf("\t%d, \t\t%d\n", pageTable[i].virtualPage, pageTable[i].physicalPage);
	}
	printf("============================================\n\n");
}
