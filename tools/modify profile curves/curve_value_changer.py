"""
Changes a profile so that all curves are fla with a single value.
"""
import xml.etree.ElementTree as ET

CURVE_POINT_VALUE = '2'
"""
NaturalPoint profile xml files are actually encoded as utf-8 but speciify in the
header that they are utf-16. This causes a problem for xml parsers, so we skip
the first line when parsing, and add it back in at the end.
"""
with open('template.xml', 'rt', newline='') as f:
    first_line = f.readline()  # Read the first line
    data = f.readlines()  # Skip the first line

root = ET.fromstringlist(data)

# Set the profile name
root.find('Name').text = CURVE_POINT_VALUE

# Set the curve value points.
for i, child in enumerate(root.iter('Curve')):
    print(child)
    for x in child.iter('Inputs'):
        try:
            iter = x.iter('Val')
            while True:
                next(iter)  # skip every first element of the pair
                val = next(iter)
                val.text = CURVE_POINT_VALUE
        except StopIteration:  # Skip the first element
            pass
with open(CURVE_POINT_VALUE + ".xml", "w", encoding="utf-8") as f:
    f.write(first_line)  # Write the first line back
    f.write(ET.tostring(root, encoding='unicode'))  # Write the modified XML
