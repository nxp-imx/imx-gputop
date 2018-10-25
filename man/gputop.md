% GPUTOP(8)
% Marius Vlad <marius-cristian.vlad@nxp.com>
% October 25, 2018

# NAME

**gputop** --  Monitor GPU clients memory, hardware counters, occupancy state
load on DMA engines, video memory and and DDR memory bandwidth (only under
Linux).

# SYNOPSIS

**gputop** [options]

**gputop** -m [mode] -- Where mode can be: **mem**, **counter_1**, **counter_2**,
**occupancy**, **dma**, **vidmem** and **ddr** (under Linux/Android).
Use this option to start **gputop** directly in a mode that you're interested on.
For **counter_1** and **counter_2** a context will be needed.
See *NOTES* section why this is necessary.

**gputop** -c ctx_no -- specify a context to attach when display context-aware
hardware counters.

**gputop** -b -- display in batch mode. For other modes than memory, this will
only take an instantaneous sample. See -f

**gputop** -f -- Use this when using **gputop** from a script.

**gputop** -x -- useful to display contexts when used with ``-b''

**gputop** -i -- ignore warnings about kernel mismatch

**gputop** -h -- display usage and help

## Interactive mode

Normally, when starting up, **gputop**, starts in interactive mode. The
following are a list of useful commands:

* 'h' -- display help page 
* '0-6'/Left-Right arrows -- switch between viewing pages
* 'x' -- display application contexts
* 'SPACE' -- select a context that you want to track. Useful for reading **counter_1** and
**counter_2** values.
* 'r' -- useful for hardware-counter pages to display different viewing modes
(switches between different modes of aggregation: MIN/MAX/AVERAGE/TIME)
* 'q'/ESC -- exits **gputop**.
* 'p' -- stops reading counter values and displays only current values. Useful
to get a instantaneous values of the counters.

# DESCRIPTION

**gputop** can be used to determine the memory usage your application is using,
or to read the hardware counters exposed by the GPU in real-time.
Additionally, DMA engines and Occupancy states are displayed. **gputop** has
multiple viewing pages: a **memory usage** page, two **hardware counter** pages,
a **DMA engine** page and an **Occupancy** page. When normally started,
**gputop** will be in interactive mode.  Type 'h' to get a list of the
current keybindings.

# REQUIREMENTS

### Linux

**gputop** requires access to **debugfs** sub-system on Linux in order to
display memory usage, used by clients submitting commands to the GPU. **gputop**
will try to mount the **debugfs** pseudo-filesystem if it is not already
mounted. In order to read hardware counters the profiler must be
activated in the driver. Usually this can be set by loading the driver
with **gpuProfiler=1 PowerManagement=0** options, or by adding a line 
to **/etc/modprobe.d/galcore.conf** with those parameters. 
See you distro manual on how to to do that.
	
### QNX

Just like in Linux, in order to be able to read the hardware counter values
**gpu-gpuProfiler** has to be set to 1 in ``graphics.conf'' file under
$GRAPHICS_ROOT directory. Other views like ``occupancy'' and DMA will require
**gpu-powerManagement** to be set to 0 (disabled).

# NOTES

## Sampling hardware-counters

**gputop** samples the driver for hardware counter values. Internally the driver
will update the values of the counters whenever the application submits a
special type of command to the GPU. Depending on how fast that happens
**gputop** can't foresee/adjust the values of the counters. So tweaking the
amount of sample taken or the delay time doesn't really help. For dealing with
situations where the application will submit either to fast or to low commands
to the GPU, several modes of viewing counters has been added. Cycle between them
to understand or get a bird-eye view of the counter values.  Empirically
MAX/AVERAGE displays the closes values to the truth.

## Context-aware counters

**counter_1** and **counter_2** are context-aware counters (i.e.: tied to an
application).

Internally the driver assigns various context IDs to the application submitting
commands to the GPU. These contexts IDs are currently required to read those
hardware counter values. Either use **-x** on the command line (together with
**-b** option and choosing **-m mem** viewing mode), or for interactive
mode use 'x' and then 'SPACE' to show and select a context ID.

In case you are getting zero'ed out values for **counter_1** 
and/or **counter_2** values, cycle thru the available counter IDs.

Due to the way the driver is built, for single-GPU core applications will have
**two** context-ids. Empirically the largest integer values holds the real
context-id.

## Unsupported GPUs

Do note, that on newer GPU cores, like GC7000 models, the behaviour is
different and it is not **only supported** in driver version newer than 6.2.4p1
(Build at least 150331).

For GCV600 (i.MX7ULP and i.MX8MM) the IDLE/LOAD register is not available hence
**gputop** will display incorrect (inversed) values.

# PAGES

## Client attached page

When viewing client attached page the following head columns are displayed:

 PID   RES(kB)   CONT(kB)   VIRT(kB)  Non-PGD(kB)  Total(kB)         CMD

* PID -- process id
* RES -- reserved memory
* CONT -- contiguous memory
* VIRT -- virtual memory
* Non-PGD -- Non-paged memory
* Total -- the sum of all above
* CMD -- the name of the application (trimmed)

These memory items correspond to memory pools in the driver.

## Vidmem page

When viewing vidmem page the following head columns are displayed for
each process.

PID    IN    VE    TE    RT    DE    BM    TS    IM    MA    SC    HZ    IC TD    FE   TFB

* IN -- index
* VE -- vertex
* TE -- texture
* RT -- render target
* DE -- depth
* BM -- bitmap
* TS -- tile status
* IM -- image
* MA -- mask
* SC -- scissor
* HZ -- hz
* IC -- i_cache
* TD -- tx_desc
* FE -- fence
* TFB -- tfb header

# EXAMPLES

When using ``-b'' option **gputop** will start in interactive mode and execute
just once its main loop. This is useful for various reason, either to get an
instantaneous view of a different viewing page, or scripting.

* Get a list of processes attached to the GPU

	$ gputop -m mem -b

* Get a list of processes attached to the GPU, but also display the contexts
ids

	$ gputop -m mem -bx

* Display counters (counter_1) using context_id

	$ gputop -m counter_1 -b -c <context_id>

* Display counters (counter_2) using context_id

	$ gputop -m counter_2 -b -c <context_id>

* Get IDLE/USAGE

	$ gputop -m occupancy -b | grep IDLE

# SEE ALSO

* under QNX see **graphics.conf** for disabling powerManagement and
enabling gpuProfiler.
* under Linux see **/sys/modules/galcore/paramenters/gpuProfiler** and
**/sys/modules/galcore/parameters/PowerManagement**.
* *libgpuperfcnt(8)*
