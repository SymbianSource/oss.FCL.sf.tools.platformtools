#
# Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies). 
# All rights reserved.
# This component and the accompanying materials are made available
# under the terms of "Eclipse Public License v1.0"
# which accompanies this distribution, and is available
# at the URL "http://www.eclipse.org/legal/epl-v10.html".
#
# Initial Contributors:
# Nokia Corporation - initial contribution.
#
# Contributors:
# 
# Description:
# 
#

##########################################################################################
# Get list of files
#

# First get SVG files from SOURCEDIR, then .bmp files from SOURCEDIR, then .svg from VECTORSOURCEDIR and finally .bmp from BITMAPSOURCEDIR
SOURCEPATHS:=$(foreach FILE, \
					$(basename $(FILES)), \
					$(firstword $(wildcard $(SOURCEDIR)$/$(FILE).svg $(SOURCEDIR)$/$(FILE).bmp $(VECTORSOURCEDIR)$/$(FILE).svg) $(BITMAPSOURCEDIR)$/$(FILE).bmp))

# Get a full list of SVG files
SVGFILES:=$(filter %.svg,$(SOURCEPATHS))

# Paths of SVGB files will be under the intermediate directory
SVGBFILES:=$(addprefix $(VECTORINTERMEDIATEDIR)$/,$(notdir $(addsuffix b,$(SVGFILES))))

# BMP files are all the other files
BMPFILES:=$(filter-out $(SVGFILES),$(SOURCEPATHS))

# Resolve mask files. Mask files are assumed to locate in the same folder than the actual bmp-file.
MASKFILES:=$(foreach FILE, \
                          $(BMPFILES), \
                          $(FILE) $(foreach MASK, \
                                            $(filter $(notdir $(basename $(FILE)))$(MASK_OPTION_1) $(notdir $(basename $(FILE)))$(MASK_OPTION_8),$(MASKFILES)), \
                                            $(dir $(FILE))$(MASK).bmp) )

# Filter out the actual bmp-files:
MASKFILES:=$(filter-out $(BMPFILES), $(MASKFILES))


##########################################################################################
# Define copy rules
#

# If SOURCEDIR is defined, we need to create a rule for copying the vector files into the intermediate directory
ifneq ($(strip $(SOURCEDIR)),)

	# Create list of files
	USERSVGFILES:=$(filter $(SOURCEDIR)%,$(SVGFILES))
	USERBMPFILES:=$(filter $(SOURCEDIR)%,$(BMPFILES))
	USERINTERMEDIATESVGFILES := $(subst $(SOURCEDIR), \
										$(VECTORINTERMEDIATEDIR), \
										$(USERSVGFILES) \
								)

	# Now define the copy rule
ifeq ($(INIT_CFG),$(PLATFORM)$(CFG))
    $(USERINTERMEDIATESVGFILES) : $(VECTORINTERMEDIATEDIR)% : $(SOURCEDIR)%
	$(TOOLCOPY) $< $@
ifeq ($(OSTYPE),cygwin)
	$(GNUCHMOD) a+rw "$@"
endif
endif
endif 


# The same copy rule is needed for system files as well, first generate the lists
SYSTEMSVGFILES:=$(filter $(VECTORSOURCEDIR)%,$(SVGFILES))
SYSTEMBMPFILES:=$(filter $(BITMAPSOURCEDIR)%,$(BMPFILES))
SYSTEMINTERMEDIATESVGFILES := $(subst $(VECTORSOURCEDIR), \
									$(VECTORINTERMEDIATEDIR), \
									$(SYSTEMSVGFILES) \
								)

# And finally define the copy rule
ifeq ($(INIT_CFG),$(PLATFORM)$(CFG))
$(SYSTEMINTERMEDIATESVGFILES) : $(VECTORINTERMEDIATEDIR)% : $(VECTORSOURCEDIR)% 
	$(TOOLCOPY) $< $@	
ifeq ($(OSTYPE),cygwin)
	$(GNUCHMOD) a+rw "$@"
endif
endif

# Get full list of intermediate files
VECTORINTERMEDIATESOURCES := $(SYSTEMINTERMEDIATESVGFILES) $(USERINTERMEDIATESVGFILES)



# Debug prints for debugging purposes
#$(warning SOURCEPATHS $(SOURCEPATHS))
#$(warning SVGFILES $(SVGFILES))
#$(warning SVGBFILES $(SVGBFILES))
#$(warning BMPFILES $(BMPFILES))
#$(warning USERSVGFILES $(USERSVGFILES))
#$(warning USERBMPFILES $(USERBMPFILES))
#$(warning USERINTERMEDIATESVGFILES $(USERINTERMEDIATESVGFILES))
#$(warning SYSTEMSVGFILES $(SYSTEMSVGFILES))
#$(warning SYSTEMBMPFILES $(SYSTEMBMPFILES))
#$(warning SYSTEMINTERMEDIATESVGFILES $(SYSTEMINTERMEDIATESVGFILES))
#$(warning VECTORINTERMEDIATESOURCES $(VECTORINTERMEDIATESOURCES))



##########################################################################################
# Rules for creating directories
#
# Please note that this implementation assumes that Mifconv core takes care of creating
# directories for files which it generates
#

# Check if the intermediate directory exists - if not, create it
ifeq ($(wildcard $(VECTORINTERMEDIATEDIR)$/),) 
	__DUMMY_MKDIR := $(shell $(TOOLMKDIR) $(VECTORINTERMEDIATEDIR)) 
endif



##########################################################################################
# Rule for calling SVGTBINENCODE ie SVG to SVGB conversion
#

%.svgb : %.svg
# If SVGENCODINGVERSION contains anything else but whitespace, provide it to svgtbinencode with -v option
	$(SVGTBINENCODE) $(addprefix -v ,$(SVGENCODINGVERSION)) $<



##########################################################################################
# MBM file name resolution. BMCONV is called by mifconv.
#

# Check if we have any bitmap sources available
ifneq ($(strip $(BMPFILES)),)
    
    # The name of the bitmap file is decided from the miffile or header file depending which one is defined
    BITMAPFILE:=$(addsuffix .mbm,$(basename $(firstword $(TARGETFILE) $(HEADERFILE))))
    #$(warning BITMAPFILE $(BITMAPFILE))

# No bitmap sources, so the filename of the MBM file is empty 
else
    BITMAPFILE:=
endif 

##########################################################################################
# This may require some explaining
# Line 5: Strip paths from SOURCEPATHS (the input files with paths)
# Line 3: For all .svg files in previous, change suffix to .svgb
# Line 2: From previous, list each item twice, once with VECTORINTERMEDIATEDIR path, and once without any path
#            Now we should have all the files in correct order in format:
#            file.bmp VECTORINTERMEDIATEDIR/file.bmp otherfile.svgb VECTORINTERMEDIATEDIR/otherfile.svgb ...
# Line 1: Use filter to select only one or either from the list above

MIFCONVFILES:=$(filter $(SVGBFILES) $(notdir $(BMPFILES)), \
                 $(foreach FILE, \
                                     $(patsubst %.svg, \
                                     %.svgb,\
                                     $(notdir $(SOURCEPATHS)) \
                  ) , \
                  $(FILE) $(VECTORINTERMEDIATEDIR)$/$(FILE) \
                  ) \
               )

MIFCONVOPTS:=$(foreach FILE,$(MIFCONVFILES),$(OPTION_$(notdir $(basename $(FILE)))) $(FILE))

