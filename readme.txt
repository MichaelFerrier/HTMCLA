===========================================================================================
HTMCLA
===========================================================================================
A C++ implementation of Numenta's Hierarchical Temporal Memory Cortical Learning Algorithm
By Michael Ferrier, 2013.
===========================================================================================

This C++ implementation is based on Numenta's CLA white paper:
https://www.groksolutions.com/htm-overview/education/HTM_CorticalLearningAlgorithms.pdf

It builds on the OpenHTM C# implementation:
https://sourceforge.net/p/openhtm/

A couple of demonstration videos are online here:
http://www.youtube.com/watch?v=IXg_XIm5kqk
http://www.youtube.com/watch?v=YeBC9eew3Lg

===========================================================================================
Build Instructions
===========================================================================================

HTMCLA depends on the Qt user interface library. The latest version can be downloaded here:
http://qt-project.org/downloads

To get Qt working and building with Visual Studio 2012 on Windows 7 64-bit, I had to follow
these steps:

1) Install QT from http://qt-project.org/downloads. Use "Qt 5.0.2 for Windows 64-bit 
   (VS 2012, 500 MB)".
2) Install QT's Visual Studio add in, from http://qt-project.org/downloads. Use "Visual 
   Studio Add-in 1.2.1 for Qt5".
3) Open VS, open QT5 menu, go to QT Options. Add the msvc2012_64 directory under the QT 
   installation (the directory with 'bin' in it).
4) In VS, under Build >> Configuration Manager, make sure Platform is set to x64. 
5) Under Project >> Properties >> Configuration Properties >> Linker >> Advanced, make sure 
   Target Machine is also set to X64.
6) Under QT5 >> QT Project Settings, set Version to msvc2012_64.
7) Create and attempt to build project. May find that you need to rename lib files being used 
   (in the QT install) to have a '5' before the dot at the end of their filenames.

I can't speak for how Qt 5 should be configured on other setups, but I find there's a lot of 
help on the web.

Once you have Qt5 integrated into your environment, you should be able to create a project in 
an environment of your choice that incorporates QT 5 and each of this project's .cpp and .h 
files. If you're using Visual Studio 2012, you should be able to use the htm.sln file.

===========================================================================================
Using HTMCLA
===========================================================================================

File Menu
=========

Load Network: Load a .xml file specifying the architecture of a CLA network. This file would 
be created in a text editor. The HTMCLA package comes with several test network files in the 
"data" subdirectory.

Load Data: You can load the segment and synapse data into a network, so that you can load up 
the state of that network after training. This is especially useful for complex learning 
tasks where training takes a long time. You can only load a data file into the same network 
from which it was saved (although the network parameters may be diffrent from what they were 
when the data was saved). Data is saved in a .clad file (CLA data).

Save Data: After training a network, you can save out its segment and synapse data so that 
you can load it directly into the same network on a later run, without having to redo the 
training. It is saved out as a .clad (CLA data) file.

Exit: Exit the program.

View Menu
=========

Update while running: If this is checked, the UI will be updated after each time step when 
running the network. This slows down execution significantly, but may be useful to watch 
progress in some cases.

Mouse Menu
==========

Toggles mouse function between "Select" and "Drag". In select mode, whatever is under the 
mouse is selected when the left button is clicked. When in drag mode, the view can be 
repositioned by being dragged. This function remains buggy.

Network Panel
=============

Displays the filename of the loaded network.

The || (Pause), > (Single Step), and >> (Continuous Run) buttons are used to control 
execution of the network.

The current time step is displayed, as well as the "Stop at:" field. If a number is entered 
here, continuous execution will cease at that given time step.

Selected Panel
==============

By left clicking on a cell or column in one of the views, that cell or column is selected. 
Information about the selected cell, column and region is displayed in this panel. If a cell 
is selected and if it has distal segments, a list of those segments is displayed at the 
bottom. If the views are displaying connections (synapses), then all of the selected column's 
proximal synapses will be shown, and all of the selected cell's distal synapses will be 
shown. To display just the distal synapses on one segment, select that segment by clicking 
on its line in the list. To deselect the segment and return to displaying all synapses on 
all distal segments, click the "Display all segments" button.

The table of segments is currently quite slow to update, so displaying the segments on a cell 
with many distal segments can cause the program to pause for a few seconds.

The Views
=========

On the left side of the UI are two views of the network. Each one can display one region or 
input space. If you'd like, they can both display the same region or input space. To select 
what it is that a view displays, select the name of the region or input space from the view's 
"Show:" menu.

A view's "Options" menu contains several options for what information is displayed in the 
view:

View Activity: Displays in the view what cells are active (orange), and what cells are 
  predicted (yellow for one-step prediction, pink for >1 step prediction).

View Top-Down Reconstruction: Displays a reconstruction of what is active in the region or 
  input space, based on what is active in the region(s) that it outputs to. Synapses are 
  traced backwards from active cells in the output region(s), to determine what cells in 
  this region (or input space) would lead to those activations.

View Top-Down Prediction: Displays a reconstruction of what is predicted to be active in the region or 
  input space during the next time step, based on what cells are 1-step predictive in the 
  region(s) that it outputs to. Synapses are traced backwards from 1-step predictive cells in 
  the output region(s), to determine what cells in this region (or input space) would lead to 
  those activations.

View Boost: When this is on, each column is colored to show its relative boost value. The 
  brightest red columns have the highest boost values, while grey columns are not boosted at 
  all.

View Connections In: When this is on, synapses from cells that input to the selected cell 
  and/or column are displayed in the view. Red synapses are not connected; green synapses are 
  connected. The brighter the color, the further the synapse's value from ConnectedPerm (in 
  either direction). Zoom in on the area to view the permanence values of the syanpses. A 
  selected column's proximal synapses are shown in it's region's input region(s) or input 
  space(s). A cell's distal synapses are shown in the cell's same region.

View Connections Out: This feature is not yet implemented. Eventually it will display the 
  synapses that output from the selected cell to other cells.

View Marked Cells: Cells can be marked so as to, for example, compare what cells are active 
  at one time step with what cells are active at a later time step. If this option is 
  selected, then a black dot will be shown over each marked cell.

Mark Active and Predicted Cells: All cells that are currently active or in predictive state 
  in the view are marked. Note that the marks will only show if "View Marked Cells" is 
  selected.

Mark Predicted Cells: All cells that are in predictive state are marked.

Mark Learning Cells: All cells that are learning cells (a subset of active cells) are marked.

The magnifying class icons and the slider between them are used to zoom in and out of the 
view. At closer zoom levels, more informaton is displayed, such as the coordinates of 
columns, the permanence values of synapses, and an "L" over learning cells.

The scrollbars may be used to reposition what part of the view is visible.

The divider between the two views can be slid from side to side to changes the sizes of the 
views. The divider can also be slid all the way to the outer edge, to hide a view. It can 
then be grabbed back from the outer edge to re-show that view.

===========================================================================================
Network Files
===========================================================================================

Network architecture is specified in a .xml file that can be created in a text editor. HTMCLA 
comes with several example network files, located in the "data" subdirectory. The network 
file specifies all parameters for each region and input space in the network, as well as any 
input patterns that an input space will use.

The network file format is based loosely on the OpenHTM network file format, with a number 
of changes. Most of the features of a network file are self explanatory by looking at the 
examples. Here are a few notes:

The synapse parameters can be specified separately for proximal and distal synapses, using 
the <ProximalSynapseParams> and <DistalSynapseParams> blocks. When used outside of any 
Region block (ie. when not nested), these parameters are used as the default for all 
proximal or distal synapses throughout all Regions. In addition to specifying network-wide 
default synapse parameters, you can specify synapse parameters specific to a Region by using 
the <ProximalSynapseParams> and <DistalSynapseParams> blocks nested within the <Region> 
block.

A network is composed of <InputSpace> and <Region> blocks. InputSpaces provide input from the 
"outside" of the network, while Regions are where the CLA learning takes place. Each Region 
must have an <Inputs> block, which specifies one or more inputs to that Region, as well as 
the input radius for that input. A Region may have any number of inputs, and each one may be 
either an InputSpace or another Region.

The <Boost> tag for a Region is used to specify the boost rate as well as the max allowed 
boost value.

The BoostingPeriod, SpatialLearningPeriod and TemporalLearningPeriod tags are used to specify 
in what range of time steps each of these types of learning is allowed to take place within 
for the Region. Typically boosting and spatial learning are ended before temporal learning 
begins for a Region, so that the temporal pooler will be working with stable spatial 
representations. However this is unnecessary if the learning rates are slow enough that the 
temporal pooler has a chance to adapt to changes in the spatial representations. The value -1 
can be used for any of these, to mean "start from the beginning" or "never stop". 

MinOverlapToReuseSegment has a "min" and a "max" value. A value within this range is chosen 
randomly for each column within the region. This allows columns to have some variation in how 
predisposed they are to re-using the same segment (and so the same learning cell) for a 
somewhat different context, versus choosing a different segment (and so a different learning 
cell). This way, some columns will be more context sensitive while others will tend to 
generalize over different contexts, providing a mix of the strengths of both approaches.

===========================================================================================
Notes on Parameters
===========================================================================================

Through experimentation I discovered a few properties about the different parameters and how 
they relate to one another:

NewNumberSynapses (aka newSynapsesCOunt in the white paper): This number should always be 
lower than the number of active cells that a cell usually has in its LocalityRadius (ie., 
that are available to be incorporated into distal segments). That way a distal segment will 
be adding synapses to a sample from the local activity, not adding *all* of the local 
activity. This is how its function is described in the Numenta docs. I found that if 
newSynapsesCount is greater than the number of active cells in localityRadius, then that 
leaves blank "slots" to be filled in later with part of a different pattern sharing some of 
the same columns, which causes lots of problems (as I described under problem #3 in the 
temporal context forking thread on the OpenHTM forum).

MinOverlapToReuseSegment (aka minThreshold in the white paper): The lower this number, the 
more likely that a similar pattern will be added to an existing segment rather than having a 
new segment created for it. So long as NewNumberSynapses is less than the typical number of 
locally active cells (as described above), this parameter doesn't need to be very high. 
MinSynapsesPerSegmentThreshold should always be <= SegmentActivationThreshold (to avoid 
problems where a cell isn't predicted or activated, but is chosen to be a learning cell). 
I've been using values between 1 and 5. As described above, I've added an option to provide 
a range for MinSynapsesPerSegmentThreshold, so that each column has a random value within 
that range chosen for its own MinSynapsesPerSegmentThreshold. This results in some columns 
being predisposed to choose the same cell to represent similar contexts, while others are 
biased to pick a different cell. This way the closer two contexts are to one another, the 
more cells the representations of those contexts will have in common, which should help with 
the learning of generalization. Providing a range like that is not necessary to fix the 
temporal forking problem however; using a higher value such as 5 results (in my tests) in 
different cells being chosen for diffrent contexts, and all of the temporal forking tests 
working correctly. Further note: This parameter is very important for the temporal context 
forking problem. Too low and the same cells will be chosen for a given pattern in different 
contexts. Too high and different cells will be chosen all the time as new context is 
learned further and further back, resulting in reuse of some of the same cells as it loops 
around (a short looping sequence such as ABBCBBA).

SegmentActivationThreshold (aka activationThreshold in the white paper): This number should 
not be lower than MinSynapsesPerSegmentThreshold, as implied in the white paper description 
for getBestMatchingSegment(): "The number of active synapses is allowed to be below 
activationThreshold, but must be above minThreshold." In my tests I have it set to 5.

PermanenceIncrease and PermanenceDecrease: If setting your parameters to allow the same 
segment to be used sometimes in similar contexts (by using a lower value for 
MinSynapsesPerSegmentThreshold), then PermanenceInc and PermanenceDec (for distal synapses) 
must be set so as to allow multiple patterns to be supported by a single segment. The higher 
the ratio of PermanenceInc/PermanenceDec, the more patterns can be supported in a single 
segment. I'm using PermanenceInc 0.01 and PermanenceDec 0.005.

===========================================================================================
Changes from the White Paper and the OpenHTM implementation
===========================================================================================

This C++ implementation of the CLA sticks pretty close to the Numenta white paper and to the 
OpenHTM implementation. However there are a few major differences that should be reviewed.

Hierarchy
=========
In this implementation, an InputSpace provides input to the network and a Region performs 
learning. The Region and InputSpace classes both inherit a common interface called DataSpace, 
which a Region uses to query its inputs. So, a Region may have any number of inputs, each of 
which can be either an InputSpace or another Region. Regions are processed in the order that 
they are defined in the network file.

Hypercolumns
============
This implementation incorporates the idea of a cortical hypercolumn, which is a group of 
columns (in the brain, about 100 columns) which all receive input from the same area of their 
inputs, and which mutually inhibit among themselves. In area V1, one hypercolumn contains 
columns that represent lines of all different orientations. The columns in a hypercolumn 
mutually inhibit among themselves, which helps to cause the different columns to specialize 
in learning to represent different features (mostly lines of different orientations) and 
which also causes only one column to be active per hypercolumn at a time. There is a 
diffrerent hypercolumn for each small patch of the visual field.

In this implementation, hypercolumns can be used by setting a region's HypercolumnDiameter. 
This defaults to 1, in which case the region acts like an OpenHTM region -- each column is 
its own hypercolumn. InhibitionRadius and PredictionRadius are specified in hypercolumns, so 
if HypercolumnDiameter is > 1, it will affect how these values are used.

A Region has an <Inhibition> tag, with a "type" attribute. If type is set to "automatic", 
inhibition will work the way it's described in the white paper and implemented in OpenHTM, 
where an inhibition radius is determined based on the average receptive field size of 
columns in the Region. If inhibition type is set to "radius", however, then a radius is 
specified, which is measured in hypercolumns. If the given radius is 0, then only columns 
within the same hypercolumn will mutually inhibit, as in V1.

Overlap
=======
In the CLA white paper and in the OpenHTM implementation, a column's Overlap value is simply 
the count of its active, connected proximal synapses, multiplied by the column's Boost value 
(a number >= 1). This is also the case in my implementation, but an additional factor is 
multipled in. That factor is the ratio of the number of connected active synapses, over 
itself plus the number of strongly connected inactive synapses. A strongly connected synapse 
is defined as a synapse with permanence > InitialPerm.

The thinking behind this is that a column which exactly represents a particular pattern of 
active inputs should be a better match for that pattern than another column that represents 
the same active inputs plus additional active inputs. For example, say a number of inputs 
are active such that they form the image of a 45 degree line. One column may have only 
enough connected synapses to exactly represent that pattern, while another column may have 
all of its synapses connected -- all of the ones that represent that line, plus every other 
synapse representing every other input. In this case, without the factor that I brought 
in, both columns would have the same overlap (given that they have the same boost value). 
That makes the columns that have most or all of their synapses connected "greedy", in the 
sense that they can represent many smaller patterns that they don't fit very well. This 
causes problems when trying to apply CLA to a vision application, and may cause problem in 
non-visual applications as well. My solution isn't perfect, but to fully address this 
problem would mean making major changes to how the spatial pooler works.

Boosting
========
Boosting is very important for its use in "separating" different patterns so that they share 
a minimum of active columns. This is necessary in order for the temporal pooler to be able to 
differentiate between different spatial paterns, and therefore to be able to make clear 
predictions.

I was unable to get useful results out of the boosting algorithm given in the CLA white paper 
or in the OpenHTM implementation. Boosting is completely implemented in Column's 
PerformBoosting() method, so a look at the code and comments in that method will help explain 
the changes I made. In short:

- If a column's ActiveDutyCycle is less than a small fraction of its MaxDutyCycle (the 
  maximum ActiveDutyCycle of all the columns in its inhibition radius), then a value (the 
  boost rate) is added to the column's Boost value. This is as described in the CLA.

- If a column's Boost rate is increased for the first non-consecutive time (ie. a new period 
  of boosting has begun for it), then each of its connected synapses have their permanence 
  decreased to ConnectedPerm. This makes it easier for the column to come to represent a 
  modified version of the pattern that it currently represents.
 
- A column's boost value is limited to the range from its MinBoost to MaxBoost. MinBoost is 
  a tiny random amount above 1.0, and MaxBoost is a tiny random amount below the Region's 
  given MaxBoost parameter value. The reason for each column having slightly different 
  MinBoost and MaxBoost values is so that multiple columns that have the same overlap and 
  have reached either MinBoost or MaxBoost, will end up having slightly different 
  (post-boosted) overlap values. This is to avoid ties between many columns that have the 
  same overlap value, which could cause many more columns than the specified 
  PercentageLocalActivity should allow, to be active at one time.

- If a column would be boosted higher, but has already reached MaxBoost, then its 
  un-connected synapse permanence values are incremented by the boost rate, but they are 
  incremented no higher than ConnectedPerm. If a column has reached MaxBoost and still needs 
  boosting because it hasn't been active, it has no hope of becoming active and so it needs 
  to change to represent a different pattern. Increasing the synapse permanence values 
  gradually gives it an increasing opportunity to represent a different pattern.

Segment Updates
===============
UpdateSegmentActiveSynapses() has been changed to fix a few problems with the original CLA 
whitepaper version.

- When short repeating loop patterns, such as AAAXAAAX... are given, the CLA quickly learns 
  the full pattern and every cell that makes up part of every representation is in 
  predictive state (1-step or higher) at all times. Because of this, no cell that's part of 
  any representation ever becomes truly inactive, and so no segment updates can ever be 
  negatively reinforced. To solve this, I made it so that if a cell goes from 1-step 
  predictive state to >1-step predictive state, then it negatively reinforces any segment 
  updates that are proven "incorrect" by this change in predictive state.

- If a cell is put in predictive state at the same time that it is activated, then in the 
  white paper version and the OpenHTM version, the segment that put it into predictive state 
  would be positively reinforced by its simultaneous activation. This should not be the case 
  however; a prediction implies that the cell will activate in the future, not at the same 
  time. (patterns of simultaneous activation are the spatial pooler's job). 
  UpdateSegmentActiveSynapses() has been modified so that it will not process such a segment 
  update in this case; the segment update will stay in the queue until a later time step, 
  when it can be determined if the prediction (of *future* activation) came true or not.

Network File Format
===================

As described in th section "Network Files", several new features have been added and made 
available in the network file format, such as synapse parameters that can be set separately 
for distal and proximal syanpses, and optionally on a per-region basis, and the ability to 
specify periods for boosting, spatial learning and temporal learning on a per-region basis. 
InputSpaces can specify a variety of different types of Patterns, such as text or bitmaps.

===========================================================================================
Getting Started
===========================================================================================

After getting HTMCLA built and running, I recommend testing the various example network files
that can be found in the "data" subdirectory. Open each one in a text editor, and you will 
find comments at the top of each file with a brief description and instructions. Most of them 
are specialized toward a particular example. net_test.xml includes many different examples, 
which can be tested by un-commenting out just the pattern that you want to test. 

===========================================================================================
Legal Information
===========================================================================================

For license information for Numenta's HTM CLA technology, and for Digia's Qt library, 
see these files in this directory:

License_Numenta.txt
License_Digia.txt

