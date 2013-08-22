% Script to process and write metadata from HDF5 data file

fileName = '../h5Data/h5Data0.h5';
[upperPath, folder] = fileparts(pwd);
XDMFname = strcat(folder, 'Fields.xdmf');
printZeroTime = 1;


%% Load and process mesh metadata
% Load mesh metadata
Info = h5info(fileName, '/MESH');

% Find number of processes in file
nProcs = size(Info.Groups(1,1).Groups, 1);

% Find number of points, cells and dataset length for each process
nPoints = zeros(nProcs, 1);
nCells = zeros(nProcs, 1);
cellLength = zeros(nProcs, 1);
procNo = zeros(nProcs, 1);
for i=1:nProcs
  cellLength(i) = Info.Groups(1,1).Groups(i,1).Datasets(1,1).Dataspace.Size;
  nPoints(i) = Info.Groups(1,1).Groups(i,1).Datasets(2,1).Dataspace.Size(2);
  nCells(i) = Info.Groups(1,1).Groups(i,1).Datasets(1,1).Attributes.Value;
  
  meshNames{i} =  Info.Groups(1,1).Groups(i,1).Name;
  procNo(i) = sscanf(meshNames{i}, '/MESH/%*f/processor%i');
end


%% Load and process field metadata
% Load mesh metadata
Info = h5info(fileName, '/FIELDS');

% Find times
nTimes = size(Info.Groups, 1);
for i=1:nTimes
  timeNames{i} = Info.Groups(i,1).Name;
  timeVals(i) = sscanf(timeNames{i}, '/FIELDS/%f');
end


% Find number of fields (in total) 
nFields =  size(Info.Groups(1,1).Groups(1,1).Datasets, 1);

% Classify and name vector- and scalarfields
fieldCmps = zeros(nFields, 1);
for i=1:nFields
  fieldNames{i} = Info.Groups(1,1).Groups(1,1).Datasets(i,1).Name;
  fieldCmps(i) = Info.Groups(1,1).Groups(1,1).Datasets(i,1).Dataspace.Size(1);
  if (fieldCmps(i) > 3)
    fieldCmps(i) = 1;
  end
end


%% Write XDMF file

% Sort time values
[T, idx] = sort(timeVals);

% Open File
fh = fopen(XDMFname, 'w');

% Print header
fprintf(fh, '<Xdmf>\n');
fprintf(fh, '  <Domain>\n');
fprintf(fh, '    <Grid Name="FieldData" GridType="Collection" CollectionType="Temporal">\n\n');

% Time loop
for t=1:nTimes
  i = idx(t);
  
  if (timeVals(i) == 0 && printZeroTime == 0)
    continue;
  end
  
  fprintf(fh, '      <Grid GridType="Collection" CollectionType="Spatial">\n');
  fprintf(fh, '        <Time Type="Single" Value="%f" />\n', timeVals(i));
  
  % Process loop
  for j=1:nProcs
    fprintf(fh, '        <Grid Name="time%f-processor%i" Type="Uniform">\n',timeVals(i), j-1);
    
    % Geometry definition
    fprintf(fh, '          <Topology Type="Mixed" NumberOfElements="%i">\n', nCells(j));
    fprintf(fh, '            <DataStructure Dimensions="%i" NumberType="Int" Format="HDF" >\n', cellLength(j));
    fprintf(fh, '              %s:%s/CELLS\n', fileName, meshNames{j});
    fprintf(fh, '            </DataStructure>\n');
    fprintf(fh, '          </Topology>\n');
    fprintf(fh, '          <Geometry GeometryType="XYZ">\n');
    fprintf(fh, '            <DataStructure Dimensions="%i 3" NumberType="Float" Presicion="8" Format="HDF" >\n', nPoints(j));
    fprintf(fh, '              %s:%s/POINTS\n', fileName, meshNames{j});
    fprintf(fh, '            </DataStructure>\n');
    fprintf(fh, '          </Geometry>\n');
    
    % Field loop
    for k=1:nFields
      
      if (fieldCmps(k) == 1)
        fprintf(fh, '          <Attribute Name="%s" Center="Cell" AttributeType="Scalar">\n', fieldNames{k});
        fprintf(fh, '            <DataStructure Format="HDF" DataType="Float" Precision="8" Dimensions="%i">\n', nCells(j));
      elseif (fieldCmps(k) == 3)
        fprintf(fh, '          <Attribute Name="%s" Center="Cell" AttributeType="Vector">\n', fieldNames{k});
        fprintf(fh, '            <DataStructure Format="HDF" DataType="Float" Precision="8" Dimensions="%i 3">\n', nCells(j));
      end
      
      fprintf(fh, '              %s:%s/processor%i/%s\n', fileName, timeNames{i}, procNo(j), fieldNames{k});
      fprintf(fh, '            </DataStructure>\n');
      fprintf(fh, '          </Attribute>\n');
    end
    
    fprintf(fh, '        </Grid>\n\n');
  end
  
  fprintf(fh, '      </Grid>\n\n\n');
end

% Print footer
fprintf(fh, '    </Grid>\n');
fprintf(fh, '  </Domain>\n');
fprintf(fh, '</Xdmf>\n');

% Close file
fclose(fh);

exit
