% Script to process and write metadata from HDF5 data file
%
% This script is now obsolete, and all functionality here is replaced by a
% Python-script 'writeXDMF.py'.


fileName = '../h5Data/h5Data0.h5';
[upperPath, folder] = fileparts(pwd);
XDMFname = strcat(folder, 'Clouds.xdmf');


%% Load and process cloud metadata
% Load mesh metadata
Info = h5info(fileName, '/CLOUDS/kinematicCloud');

% Find number of time steps present
nTimes = size(Info.Groups, 1);

% Extraxt some useful information from file
nPoints = zeros(nTimes, 1);
timeVals = zeros(nTimes, 1);
for i=1:nTimes
  timeNames{i} = Info.Groups(i,1).Name;
  timeVals(i) = sscanf(timeNames{i}, '/CLOUDS/kinematicCloud/%f');
  nPoints(i) =  Info.Groups(i,1).Datasets(1,1).Dataspace.Size(1,2);
end


%% Write XDMF file

% Sort time values
[T, idx] = sort(timeVals);

% Open File
fh = fopen(XDMFname, 'w');

% Print header
fprintf(fh, '<Xdmf>\n');
fprintf(fh, '  <Domain>\n');
fprintf(fh, '    <Grid Name="cloudData" GridType="Collection" CollectionType="Temporal">\n\n');

% Time loop
for t=1:nTimes
  i = idx(t);
  
  fprintf(fh, '      <Grid Name="time%f" Type="Uniform">\n',timeVals(i));
  fprintf(fh, '        <Time Type="Single" Value="%f" />\n', timeVals(i));
    
  % Geometry definition
  fprintf(fh, '        <Geometry GeometryType="XYZ">\n');
  fprintf(fh, '          <DataStructure Dimensions="%i 3" NumberType="Float" Presicion="4" Format="HDF" >\n', nPoints(i));
  fprintf(fh, '            %s:%s/position\n', fileName, timeNames{i});
  fprintf(fh, '          </DataStructure>\n');
  fprintf(fh, '        </Geometry>\n');
  
  fprintf(fh, '        <Topology Type="Polyvertex" NodesPerElement="1" NumberOfElements="%i">\n', nPoints(i));
  fprintf(fh, '        </Topology>\n');
  
  % Velocity
  fprintf(fh, '        <Attribute Name="U" Center="Node" AttributeType="Vector">\n');
  fprintf(fh, '          <DataStructure Format="HDF" DataType="Float" Precision="4" Dimensions="%i 3">\n', nPoints(i));
  fprintf(fh, '            %s:%s/U\n', fileName, timeNames{i});
  fprintf(fh, '          </DataStructure>\n');
  fprintf(fh, '        </Attribute>\n');
  
  % Slip velocity
  fprintf(fh, '        <Attribute Name="Us" Center="Node" AttributeType="Vector">\n');
  fprintf(fh, '          <DataStructure Format="HDF" DataType="Float" Precision="4" Dimensions="%i 3">\n', nPoints(i));
  fprintf(fh, '            %s:%s/Us\n', fileName, timeNames{i});
  fprintf(fh, '          </DataStructure>\n');
  fprintf(fh, '        </Attribute>\n');
  
  % Age
  fprintf(fh, '        <Attribute Name="age" Center="Node" AttributeType="Scalar">\n');
  fprintf(fh, '          <DataStructure Format="HDF" DataType="Float" Precision="4" Dimensions="%i">\n', nPoints(i));
  fprintf(fh, '            %s:%s/age\n', fileName, timeNames{i});
  fprintf(fh, '          </DataStructure>\n');
  fprintf(fh, '        </Attribute>\n');
  
  fprintf(fh, '      </Grid>\n\n\n');
end

% Print footer
fprintf(fh, '    </Grid>\n');
fprintf(fh, '  </Domain>\n');
fprintf(fh, '</Xdmf>\n');

% Close file
fclose(fh);

exit
