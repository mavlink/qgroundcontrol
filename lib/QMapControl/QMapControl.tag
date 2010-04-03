<?xml version='1.0' encoding='ISO-8859-1' standalone='yes' ?>
<tagfile>
  <compound kind="namespace">
    <name>qmapcontrol</name>
    <filename>namespaceqmapcontrol.html</filename>
    <class kind="class">qmapcontrol::CirclePoint</class>
    <class kind="class">qmapcontrol::Curve</class>
    <class kind="class">qmapcontrol::EmptyMapAdapter</class>
    <class kind="class">qmapcontrol::FixedImageOverlay</class>
    <class kind="class">qmapcontrol::Geometry</class>
    <class kind="class">qmapcontrol::GeometryLayer</class>
    <class kind="class">qmapcontrol::GoogleMapAdapter</class>
    <class kind="class">qmapcontrol::GoogleSatMapAdapter</class>
    <class kind="class">qmapcontrol::GPS_Position</class>
    <class kind="class">qmapcontrol::ImagePoint</class>
    <class kind="class">qmapcontrol::Layer</class>
    <class kind="class">qmapcontrol::LineString</class>
    <class kind="class">qmapcontrol::MapAdapter</class>
    <class kind="class">qmapcontrol::MapControl</class>
    <class kind="class">qmapcontrol::MapLayer</class>
    <class kind="class">qmapcontrol::MapNetwork</class>
    <class kind="class">qmapcontrol::OpenAerialMapAdapter</class>
    <class kind="class">qmapcontrol::OSMMapAdapter</class>
    <class kind="class">qmapcontrol::Point</class>
    <class kind="class">qmapcontrol::TileMapAdapter</class>
    <class kind="class">qmapcontrol::WMSMapAdapter</class>
    <class kind="class">qmapcontrol::YahooMapAdapter</class>
  </compound>
  <compound kind="class">
    <name>qmapcontrol::CirclePoint</name>
    <filename>classqmapcontrol_1_1CirclePoint.html</filename>
    <base>qmapcontrol::Point</base>
    <member kind="enumeration">
      <name>Alignment</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>cdfaca60ec19c0265bac2692d7982726</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TopLeft</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>cdfaca60ec19c0265bac2692d798272661f66ddc6702462a94d3e231f02b9017</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TopRight</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>cdfaca60ec19c0265bac2692d79827267e42a96f07eab63a8c9fa8a0526f34f4</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>BottomLeft</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>cdfaca60ec19c0265bac2692d7982726e61b9b6ea2fa75ca500d5bb1eaf6f6fc</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>BottomRight</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>cdfaca60ec19c0265bac2692d79827261640f649d644701a2f4633e6bd88b20c</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>Middle</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>cdfaca60ec19c0265bac2692d7982726673e6efef9aafe98078c5552e99c923c</anchor>
      <arglist></arglist>
    </member>
    <member kind="slot" virtualness="virtual">
      <type>virtual void</type>
      <name>setVisible</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>18e44e30b31525a243960ca3928125aa</anchor>
      <arglist>(bool visible)</arglist>
    </member>
    <member kind="signal">
      <type>void</type>
      <name>geometryClicked</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>685dcab83356e5cc449475f177bb487d</anchor>
      <arglist>(Geometry *geometry, QPoint point)</arglist>
    </member>
    <member kind="signal">
      <type>void</type>
      <name>positionChanged</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>807f9cfb1b9d680ca76cf825cc9cf46a</anchor>
      <arglist>(Geometry *geom)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual QRectF</type>
      <name>boundingBox</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>cbb256b5f9f888e9cd3bb475108ece24</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>CirclePoint</name>
      <anchorfile>classqmapcontrol_1_1CirclePoint.html</anchorfile>
      <anchor>13300765d52da11cc8cbb4384e8e9e23</anchor>
      <arglist>(qreal x, qreal y, int radius=10, QString name=QString(), Alignment alignment=Middle, QPen *pen=0)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>CirclePoint</name>
      <anchorfile>classqmapcontrol_1_1CirclePoint.html</anchorfile>
      <anchor>a0dd3496708e507c8185d5ae5f5e79ad</anchor>
      <arglist>(qreal x, qreal y, QString name=QString(), Alignment alignment=Middle, QPen *pen=0)</arglist>
    </member>
    <member kind="function">
      <type>QPointF</type>
      <name>coordinate</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>2fbb44b2ed047287d715484d2fda7299</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>Equals</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>029a8b50c439c719aac173bffe4cfb71</anchor>
      <arglist>(Geometry *geom)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>isVisible</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>08422ee75ab02691943c1ca87e2bc563</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>qreal</type>
      <name>latitude</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>6311aabecac471455760aae4790cff91</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>qreal</type>
      <name>longitude</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>2b0f7ec9068af09bcf151af61a785845</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>QString</type>
      <name>name</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>2b0a198f837184bf6fff555cee3ce770</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>Geometry *</type>
      <name>parentGeometry</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>771cc513dc079219d5da2c4b81019d7c</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>QPen *</type>
      <name>pen</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>ed7be2fcd2c1d7bccb55f5ac73d7a662</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>QPixmap *</type>
      <name>pixmap</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>166d4272301d85cc53e114ce4d26caf5</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>setBaselevel</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>91f1496833bfda9f7a7ec5fcb02a1895</anchor>
      <arglist>(int zoomlevel)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>setMaxsize</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>dc2724c4e195727b823ff55c940283de</anchor>
      <arglist>(QSize maxsize)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>setMinsize</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>c40b3e44f54fab1330b9309ac7bd84d2</anchor>
      <arglist>(QSize minsize)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>setName</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>6220fae15759fd0fa7d75e415df34e83</anchor>
      <arglist>(QString name)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual void</type>
      <name>setPen</name>
      <anchorfile>classqmapcontrol_1_1CirclePoint.html</anchorfile>
      <anchor>a92f0f1b5d2fd424196a33012ffe8ea1</anchor>
      <arglist>(QPen *pen)</arglist>
    </member>
    <member kind="function">
      <type>QString</type>
      <name>toString</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>3a013a6edb6d10a71297978bc31a796b</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>QWidget *</type>
      <name>widget</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>d1eaabeb2b227cd055ccf4b4e2818480</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" protection="protected" virtualness="virtual">
      <type>virtual bool</type>
      <name>Touches</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>73504541bf9e21ceceb82c6b4c3f2b04</anchor>
      <arglist>(Point *geom, const MapAdapter *mapadapter)</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>qmapcontrol::Curve</name>
    <filename>classqmapcontrol_1_1Curve.html</filename>
    <base>qmapcontrol::Geometry</base>
    <member kind="slot" virtualness="virtual">
      <type>virtual void</type>
      <name>setVisible</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>18e44e30b31525a243960ca3928125aa</anchor>
      <arglist>(bool visible)</arglist>
    </member>
    <member kind="signal">
      <type>void</type>
      <name>geometryClicked</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>685dcab83356e5cc449475f177bb487d</anchor>
      <arglist>(Geometry *geometry, QPoint point)</arglist>
    </member>
    <member kind="signal">
      <type>void</type>
      <name>positionChanged</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>807f9cfb1b9d680ca76cf825cc9cf46a</anchor>
      <arglist>(Geometry *geom)</arglist>
    </member>
    <member kind="function" virtualness="pure">
      <type>virtual QRectF</type>
      <name>boundingBox</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>f92c4fa46f711bea92efe5ab80f9084d</anchor>
      <arglist>()=0</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>Equals</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>029a8b50c439c719aac173bffe4cfb71</anchor>
      <arglist>(Geometry *geom)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>isVisible</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>08422ee75ab02691943c1ca87e2bc563</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>QString</type>
      <name>name</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>2b0a198f837184bf6fff555cee3ce770</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>Geometry *</type>
      <name>parentGeometry</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>771cc513dc079219d5da2c4b81019d7c</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>QPen *</type>
      <name>pen</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>ed7be2fcd2c1d7bccb55f5ac73d7a662</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>setName</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>6220fae15759fd0fa7d75e415df34e83</anchor>
      <arglist>(QString name)</arglist>
    </member>
    <member kind="function">
      <type>QString</type>
      <name>toString</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>3a013a6edb6d10a71297978bc31a796b</anchor>
      <arglist>()</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>qmapcontrol::EmptyMapAdapter</name>
    <filename>classqmapcontrol_1_1EmptyMapAdapter.html</filename>
    <base>qmapcontrol::MapAdapter</base>
    <member kind="function" virtualness="virtual">
      <type>virtual QPoint</type>
      <name>coordinateToDisplay</name>
      <anchorfile>classqmapcontrol_1_1EmptyMapAdapter.html</anchorfile>
      <anchor>94134b06e350d302f5b3a63f0016aa60</anchor>
      <arglist>(const QPointF &amp;) const </arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>currentZoom</name>
      <anchorfile>classqmapcontrol_1_1MapAdapter.html</anchorfile>
      <anchor>8c7b803b9faa35db237e40c361e1c036</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual QPointF</type>
      <name>displayToCoordinate</name>
      <anchorfile>classqmapcontrol_1_1EmptyMapAdapter.html</anchorfile>
      <anchor>601b4631d9d891eabffb063c510cc088</anchor>
      <arglist>(const QPoint &amp;) const </arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>EmptyMapAdapter</name>
      <anchorfile>classqmapcontrol_1_1EmptyMapAdapter.html</anchorfile>
      <anchor>9208e1a9da209564f85a50318cda7310</anchor>
      <arglist>(int tileSize=256, int minZoom=0, int maxZoom=17)</arglist>
    </member>
    <member kind="function">
      <type>QString</type>
      <name>host</name>
      <anchorfile>classqmapcontrol_1_1MapAdapter.html</anchorfile>
      <anchor>7ee8388c7880d8a3466967464f5034b6</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>maxZoom</name>
      <anchorfile>classqmapcontrol_1_1MapAdapter.html</anchorfile>
      <anchor>3a33e897bc474405772d17a7c81f8747</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>minZoom</name>
      <anchorfile>classqmapcontrol_1_1MapAdapter.html</anchorfile>
      <anchor>7457f51db57914a85bbefcc8c9fa97b4</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>tilesize</name>
      <anchorfile>classqmapcontrol_1_1MapAdapter.html</anchorfile>
      <anchor>f105c5a0cf588a3f60d67744bd353391</anchor>
      <arglist>() const </arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>qmapcontrol::FixedImageOverlay</name>
    <filename>classqmapcontrol_1_1FixedImageOverlay.html</filename>
    <base>qmapcontrol::ImagePoint</base>
    <member kind="enumeration">
      <name>Alignment</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>cdfaca60ec19c0265bac2692d7982726</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TopLeft</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>cdfaca60ec19c0265bac2692d798272661f66ddc6702462a94d3e231f02b9017</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TopRight</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>cdfaca60ec19c0265bac2692d79827267e42a96f07eab63a8c9fa8a0526f34f4</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>BottomLeft</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>cdfaca60ec19c0265bac2692d7982726e61b9b6ea2fa75ca500d5bb1eaf6f6fc</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>BottomRight</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>cdfaca60ec19c0265bac2692d79827261640f649d644701a2f4633e6bd88b20c</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>Middle</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>cdfaca60ec19c0265bac2692d7982726673e6efef9aafe98078c5552e99c923c</anchor>
      <arglist></arglist>
    </member>
    <member kind="slot" virtualness="virtual">
      <type>virtual void</type>
      <name>setVisible</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>18e44e30b31525a243960ca3928125aa</anchor>
      <arglist>(bool visible)</arglist>
    </member>
    <member kind="signal">
      <type>void</type>
      <name>geometryClicked</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>685dcab83356e5cc449475f177bb487d</anchor>
      <arglist>(Geometry *geometry, QPoint point)</arglist>
    </member>
    <member kind="signal">
      <type>void</type>
      <name>positionChanged</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>807f9cfb1b9d680ca76cf825cc9cf46a</anchor>
      <arglist>(Geometry *geom)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual QRectF</type>
      <name>boundingBox</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>cbb256b5f9f888e9cd3bb475108ece24</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>QPointF</type>
      <name>coordinate</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>2fbb44b2ed047287d715484d2fda7299</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>Equals</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>029a8b50c439c719aac173bffe4cfb71</anchor>
      <arglist>(Geometry *geom)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>FixedImageOverlay</name>
      <anchorfile>classqmapcontrol_1_1FixedImageOverlay.html</anchorfile>
      <anchor>a3cc273119f113f1ba255315b9327d0e</anchor>
      <arglist>(qreal x_upperleft, qreal y_upperleft, qreal x_lowerright, qreal y_lowerright, QPixmap *pixmap, QString name=QString())</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>FixedImageOverlay</name>
      <anchorfile>classqmapcontrol_1_1FixedImageOverlay.html</anchorfile>
      <anchor>621806ec022f1b35a2383b64787a5827</anchor>
      <arglist>(qreal x_upperleft, qreal y_upperleft, qreal x_lowerright, qreal y_lowerright, QString filename, QString name=QString())</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>isVisible</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>08422ee75ab02691943c1ca87e2bc563</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>qreal</type>
      <name>latitude</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>6311aabecac471455760aae4790cff91</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>qreal</type>
      <name>longitude</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>2b0f7ec9068af09bcf151af61a785845</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>QString</type>
      <name>name</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>2b0a198f837184bf6fff555cee3ce770</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>Geometry *</type>
      <name>parentGeometry</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>771cc513dc079219d5da2c4b81019d7c</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>QPen *</type>
      <name>pen</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>ed7be2fcd2c1d7bccb55f5ac73d7a662</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>QPixmap *</type>
      <name>pixmap</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>166d4272301d85cc53e114ce4d26caf5</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>setBaselevel</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>91f1496833bfda9f7a7ec5fcb02a1895</anchor>
      <arglist>(int zoomlevel)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>setMaxsize</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>dc2724c4e195727b823ff55c940283de</anchor>
      <arglist>(QSize maxsize)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>setMinsize</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>c40b3e44f54fab1330b9309ac7bd84d2</anchor>
      <arglist>(QSize minsize)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>setName</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>6220fae15759fd0fa7d75e415df34e83</anchor>
      <arglist>(QString name)</arglist>
    </member>
    <member kind="function">
      <type>QString</type>
      <name>toString</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>3a013a6edb6d10a71297978bc31a796b</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>QWidget *</type>
      <name>widget</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>d1eaabeb2b227cd055ccf4b4e2818480</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" protection="protected" virtualness="virtual">
      <type>virtual bool</type>
      <name>Touches</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>73504541bf9e21ceceb82c6b4c3f2b04</anchor>
      <arglist>(Point *geom, const MapAdapter *mapadapter)</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>qmapcontrol::Geometry</name>
    <filename>classqmapcontrol_1_1Geometry.html</filename>
    <member kind="slot" virtualness="virtual">
      <type>virtual void</type>
      <name>setVisible</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>18e44e30b31525a243960ca3928125aa</anchor>
      <arglist>(bool visible)</arglist>
    </member>
    <member kind="signal">
      <type>void</type>
      <name>geometryClicked</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>685dcab83356e5cc449475f177bb487d</anchor>
      <arglist>(Geometry *geometry, QPoint point)</arglist>
    </member>
    <member kind="signal">
      <type>void</type>
      <name>positionChanged</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>807f9cfb1b9d680ca76cf825cc9cf46a</anchor>
      <arglist>(Geometry *geom)</arglist>
    </member>
    <member kind="function" virtualness="pure">
      <type>virtual QRectF</type>
      <name>boundingBox</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>f92c4fa46f711bea92efe5ab80f9084d</anchor>
      <arglist>()=0</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>Equals</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>029a8b50c439c719aac173bffe4cfb71</anchor>
      <arglist>(Geometry *geom)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>isVisible</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>08422ee75ab02691943c1ca87e2bc563</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>QString</type>
      <name>name</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>2b0a198f837184bf6fff555cee3ce770</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>Geometry *</type>
      <name>parentGeometry</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>771cc513dc079219d5da2c4b81019d7c</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>QPen *</type>
      <name>pen</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>ed7be2fcd2c1d7bccb55f5ac73d7a662</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>setName</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>6220fae15759fd0fa7d75e415df34e83</anchor>
      <arglist>(QString name)</arglist>
    </member>
    <member kind="function">
      <type>QString</type>
      <name>toString</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>3a013a6edb6d10a71297978bc31a796b</anchor>
      <arglist>()</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>qmapcontrol::GeometryLayer</name>
    <filename>classqmapcontrol_1_1GeometryLayer.html</filename>
    <base>qmapcontrol::Layer</base>
    <member kind="enumeration">
      <name>LayerType</name>
      <anchorfile>classqmapcontrol_1_1Layer.html</anchorfile>
      <anchor>56943a0946e5f15e5e58054b8e7a04a4</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>MapLayer</name>
      <anchorfile>classqmapcontrol_1_1Layer.html</anchorfile>
      <anchor>56943a0946e5f15e5e58054b8e7a04a4fe7df421203e4175d260b8dabcbe3002</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>GeometryLayer</name>
      <anchorfile>classqmapcontrol_1_1Layer.html</anchorfile>
      <anchor>56943a0946e5f15e5e58054b8e7a04a46c04bd58c42df8a7539aba782503fee0</anchor>
      <arglist></arglist>
    </member>
    <member kind="slot">
      <type>void</type>
      <name>setVisible</name>
      <anchorfile>classqmapcontrol_1_1Layer.html</anchorfile>
      <anchor>18e44e30b31525a243960ca3928125aa</anchor>
      <arglist>(bool visible)</arglist>
    </member>
    <member kind="signal">
      <type>void</type>
      <name>geometryClicked</name>
      <anchorfile>classqmapcontrol_1_1Layer.html</anchorfile>
      <anchor>685dcab83356e5cc449475f177bb487d</anchor>
      <arglist>(Geometry *geometry, QPoint point)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>addGeometry</name>
      <anchorfile>classqmapcontrol_1_1Layer.html</anchorfile>
      <anchor>b692d7d08414ed2b744946b88872827f</anchor>
      <arglist>(Geometry *geometry)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>clearGeometries</name>
      <anchorfile>classqmapcontrol_1_1Layer.html</anchorfile>
      <anchor>0b551ef8f4d0fd26cfa2ed0ec4c981b3</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>GeometryLayer</name>
      <anchorfile>classqmapcontrol_1_1GeometryLayer.html</anchorfile>
      <anchor>64e2ab047db14f0d86424bee947c94af</anchor>
      <arglist>(QString layername, MapAdapter *mapadapter, bool takeevents=true)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>isVisible</name>
      <anchorfile>classqmapcontrol_1_1Layer.html</anchorfile>
      <anchor>08422ee75ab02691943c1ca87e2bc563</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>QString</type>
      <name>layername</name>
      <anchorfile>classqmapcontrol_1_1Layer.html</anchorfile>
      <anchor>414e94fdd70490d75ddccb6923ae3410</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>Layer::LayerType</type>
      <name>layertype</name>
      <anchorfile>classqmapcontrol_1_1Layer.html</anchorfile>
      <anchor>1cfbd8a5c27cf9cb400fa458a1f70ba5</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>const MapAdapter *</type>
      <name>mapadapter</name>
      <anchorfile>classqmapcontrol_1_1Layer.html</anchorfile>
      <anchor>23cb6462136a0920739cdeb5f1709fa7</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>removeGeometry</name>
      <anchorfile>classqmapcontrol_1_1Layer.html</anchorfile>
      <anchor>b34ffa138d7262b801cdfedaf4a129d3</anchor>
      <arglist>(Geometry *geometry)</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>qmapcontrol::GoogleMapAdapter</name>
    <filename>classqmapcontrol_1_1GoogleMapAdapter.html</filename>
    <base>qmapcontrol::TileMapAdapter</base>
    <member kind="function" virtualness="virtual">
      <type>virtual QPoint</type>
      <name>coordinateToDisplay</name>
      <anchorfile>classqmapcontrol_1_1TileMapAdapter.html</anchorfile>
      <anchor>94134b06e350d302f5b3a63f0016aa60</anchor>
      <arglist>(const QPointF &amp;) const </arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>currentZoom</name>
      <anchorfile>classqmapcontrol_1_1MapAdapter.html</anchorfile>
      <anchor>8c7b803b9faa35db237e40c361e1c036</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual QPointF</type>
      <name>displayToCoordinate</name>
      <anchorfile>classqmapcontrol_1_1TileMapAdapter.html</anchorfile>
      <anchor>601b4631d9d891eabffb063c510cc088</anchor>
      <arglist>(const QPoint &amp;) const </arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>GoogleMapAdapter</name>
      <anchorfile>classqmapcontrol_1_1GoogleMapAdapter.html</anchorfile>
      <anchor>cce53053ca2cbe6b91836ee24fc98b22</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>QString</type>
      <name>host</name>
      <anchorfile>classqmapcontrol_1_1MapAdapter.html</anchorfile>
      <anchor>7ee8388c7880d8a3466967464f5034b6</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>maxZoom</name>
      <anchorfile>classqmapcontrol_1_1MapAdapter.html</anchorfile>
      <anchor>3a33e897bc474405772d17a7c81f8747</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>minZoom</name>
      <anchorfile>classqmapcontrol_1_1MapAdapter.html</anchorfile>
      <anchor>7457f51db57914a85bbefcc8c9fa97b4</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>tilesize</name>
      <anchorfile>classqmapcontrol_1_1MapAdapter.html</anchorfile>
      <anchor>f105c5a0cf588a3f60d67744bd353391</anchor>
      <arglist>() const </arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>qmapcontrol::GoogleSatMapAdapter</name>
    <filename>classqmapcontrol_1_1GoogleSatMapAdapter.html</filename>
    <base>qmapcontrol::TileMapAdapter</base>
    <member kind="function" virtualness="virtual">
      <type>virtual QPoint</type>
      <name>coordinateToDisplay</name>
      <anchorfile>classqmapcontrol_1_1GoogleSatMapAdapter.html</anchorfile>
      <anchor>94134b06e350d302f5b3a63f0016aa60</anchor>
      <arglist>(const QPointF &amp;) const </arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>currentZoom</name>
      <anchorfile>classqmapcontrol_1_1MapAdapter.html</anchorfile>
      <anchor>8c7b803b9faa35db237e40c361e1c036</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual QPointF</type>
      <name>displayToCoordinate</name>
      <anchorfile>classqmapcontrol_1_1GoogleSatMapAdapter.html</anchorfile>
      <anchor>601b4631d9d891eabffb063c510cc088</anchor>
      <arglist>(const QPoint &amp;) const </arglist>
    </member>
    <member kind="function">
      <type>QString</type>
      <name>getHost</name>
      <anchorfile>classqmapcontrol_1_1GoogleSatMapAdapter.html</anchorfile>
      <anchor>47a82056e29973632b3f3323c1fa5e30</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>GoogleSatMapAdapter</name>
      <anchorfile>classqmapcontrol_1_1GoogleSatMapAdapter.html</anchorfile>
      <anchor>3f7fb833b8e1325d5164c88e07425b94</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>QString</type>
      <name>host</name>
      <anchorfile>classqmapcontrol_1_1MapAdapter.html</anchorfile>
      <anchor>7ee8388c7880d8a3466967464f5034b6</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>maxZoom</name>
      <anchorfile>classqmapcontrol_1_1MapAdapter.html</anchorfile>
      <anchor>3a33e897bc474405772d17a7c81f8747</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>minZoom</name>
      <anchorfile>classqmapcontrol_1_1MapAdapter.html</anchorfile>
      <anchor>7457f51db57914a85bbefcc8c9fa97b4</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>tilesize</name>
      <anchorfile>classqmapcontrol_1_1MapAdapter.html</anchorfile>
      <anchor>f105c5a0cf588a3f60d67744bd353391</anchor>
      <arglist>() const </arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>qmapcontrol::ImagePoint</name>
    <filename>classqmapcontrol_1_1ImagePoint.html</filename>
    <base>qmapcontrol::Point</base>
    <member kind="enumeration">
      <name>Alignment</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>cdfaca60ec19c0265bac2692d7982726</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TopLeft</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>cdfaca60ec19c0265bac2692d798272661f66ddc6702462a94d3e231f02b9017</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TopRight</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>cdfaca60ec19c0265bac2692d79827267e42a96f07eab63a8c9fa8a0526f34f4</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>BottomLeft</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>cdfaca60ec19c0265bac2692d7982726e61b9b6ea2fa75ca500d5bb1eaf6f6fc</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>BottomRight</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>cdfaca60ec19c0265bac2692d79827261640f649d644701a2f4633e6bd88b20c</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>Middle</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>cdfaca60ec19c0265bac2692d7982726673e6efef9aafe98078c5552e99c923c</anchor>
      <arglist></arglist>
    </member>
    <member kind="slot" virtualness="virtual">
      <type>virtual void</type>
      <name>setVisible</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>18e44e30b31525a243960ca3928125aa</anchor>
      <arglist>(bool visible)</arglist>
    </member>
    <member kind="signal">
      <type>void</type>
      <name>geometryClicked</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>685dcab83356e5cc449475f177bb487d</anchor>
      <arglist>(Geometry *geometry, QPoint point)</arglist>
    </member>
    <member kind="signal">
      <type>void</type>
      <name>positionChanged</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>807f9cfb1b9d680ca76cf825cc9cf46a</anchor>
      <arglist>(Geometry *geom)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual QRectF</type>
      <name>boundingBox</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>cbb256b5f9f888e9cd3bb475108ece24</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>QPointF</type>
      <name>coordinate</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>2fbb44b2ed047287d715484d2fda7299</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>Equals</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>029a8b50c439c719aac173bffe4cfb71</anchor>
      <arglist>(Geometry *geom)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>ImagePoint</name>
      <anchorfile>classqmapcontrol_1_1ImagePoint.html</anchorfile>
      <anchor>27a768be97d6147e8a7ac92f23ce4a48</anchor>
      <arglist>(qreal x, qreal y, QPixmap *pixmap, QString name=QString(), Alignment alignment=Middle)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>ImagePoint</name>
      <anchorfile>classqmapcontrol_1_1ImagePoint.html</anchorfile>
      <anchor>a5121dbb37cf1b8924a376e4c7edd728</anchor>
      <arglist>(qreal x, qreal y, QString filename, QString name=QString(), Alignment alignment=Middle)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>isVisible</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>08422ee75ab02691943c1ca87e2bc563</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>qreal</type>
      <name>latitude</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>6311aabecac471455760aae4790cff91</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>qreal</type>
      <name>longitude</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>2b0f7ec9068af09bcf151af61a785845</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>QString</type>
      <name>name</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>2b0a198f837184bf6fff555cee3ce770</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>Geometry *</type>
      <name>parentGeometry</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>771cc513dc079219d5da2c4b81019d7c</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>QPen *</type>
      <name>pen</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>ed7be2fcd2c1d7bccb55f5ac73d7a662</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>QPixmap *</type>
      <name>pixmap</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>166d4272301d85cc53e114ce4d26caf5</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>setBaselevel</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>91f1496833bfda9f7a7ec5fcb02a1895</anchor>
      <arglist>(int zoomlevel)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>setMaxsize</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>dc2724c4e195727b823ff55c940283de</anchor>
      <arglist>(QSize maxsize)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>setMinsize</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>c40b3e44f54fab1330b9309ac7bd84d2</anchor>
      <arglist>(QSize minsize)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>setName</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>6220fae15759fd0fa7d75e415df34e83</anchor>
      <arglist>(QString name)</arglist>
    </member>
    <member kind="function">
      <type>QString</type>
      <name>toString</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>3a013a6edb6d10a71297978bc31a796b</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>QWidget *</type>
      <name>widget</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>d1eaabeb2b227cd055ccf4b4e2818480</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" protection="protected" virtualness="virtual">
      <type>virtual bool</type>
      <name>Touches</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>73504541bf9e21ceceb82c6b4c3f2b04</anchor>
      <arglist>(Point *geom, const MapAdapter *mapadapter)</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>qmapcontrol::Layer</name>
    <filename>classqmapcontrol_1_1Layer.html</filename>
    <member kind="enumeration">
      <name>LayerType</name>
      <anchorfile>classqmapcontrol_1_1Layer.html</anchorfile>
      <anchor>56943a0946e5f15e5e58054b8e7a04a4</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>MapLayer</name>
      <anchorfile>classqmapcontrol_1_1Layer.html</anchorfile>
      <anchor>56943a0946e5f15e5e58054b8e7a04a4fe7df421203e4175d260b8dabcbe3002</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>GeometryLayer</name>
      <anchorfile>classqmapcontrol_1_1Layer.html</anchorfile>
      <anchor>56943a0946e5f15e5e58054b8e7a04a46c04bd58c42df8a7539aba782503fee0</anchor>
      <arglist></arglist>
    </member>
    <member kind="slot">
      <type>void</type>
      <name>setVisible</name>
      <anchorfile>classqmapcontrol_1_1Layer.html</anchorfile>
      <anchor>18e44e30b31525a243960ca3928125aa</anchor>
      <arglist>(bool visible)</arglist>
    </member>
    <member kind="signal">
      <type>void</type>
      <name>geometryClicked</name>
      <anchorfile>classqmapcontrol_1_1Layer.html</anchorfile>
      <anchor>685dcab83356e5cc449475f177bb487d</anchor>
      <arglist>(Geometry *geometry, QPoint point)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>addGeometry</name>
      <anchorfile>classqmapcontrol_1_1Layer.html</anchorfile>
      <anchor>b692d7d08414ed2b744946b88872827f</anchor>
      <arglist>(Geometry *geometry)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>clearGeometries</name>
      <anchorfile>classqmapcontrol_1_1Layer.html</anchorfile>
      <anchor>0b551ef8f4d0fd26cfa2ed0ec4c981b3</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>isVisible</name>
      <anchorfile>classqmapcontrol_1_1Layer.html</anchorfile>
      <anchor>08422ee75ab02691943c1ca87e2bc563</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Layer</name>
      <anchorfile>classqmapcontrol_1_1Layer.html</anchorfile>
      <anchor>7ffb001076dc500ad13e523836bda23d</anchor>
      <arglist>(QString layername, MapAdapter *mapadapter, enum LayerType layertype, bool takeevents=true)</arglist>
    </member>
    <member kind="function">
      <type>QString</type>
      <name>layername</name>
      <anchorfile>classqmapcontrol_1_1Layer.html</anchorfile>
      <anchor>414e94fdd70490d75ddccb6923ae3410</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>Layer::LayerType</type>
      <name>layertype</name>
      <anchorfile>classqmapcontrol_1_1Layer.html</anchorfile>
      <anchor>1cfbd8a5c27cf9cb400fa458a1f70ba5</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>const MapAdapter *</type>
      <name>mapadapter</name>
      <anchorfile>classqmapcontrol_1_1Layer.html</anchorfile>
      <anchor>23cb6462136a0920739cdeb5f1709fa7</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>removeGeometry</name>
      <anchorfile>classqmapcontrol_1_1Layer.html</anchorfile>
      <anchor>b34ffa138d7262b801cdfedaf4a129d3</anchor>
      <arglist>(Geometry *geometry)</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>qmapcontrol::LineString</name>
    <filename>classqmapcontrol_1_1LineString.html</filename>
    <base>qmapcontrol::Curve</base>
    <member kind="slot" virtualness="virtual">
      <type>virtual void</type>
      <name>setVisible</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>18e44e30b31525a243960ca3928125aa</anchor>
      <arglist>(bool visible)</arglist>
    </member>
    <member kind="signal">
      <type>void</type>
      <name>geometryClicked</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>685dcab83356e5cc449475f177bb487d</anchor>
      <arglist>(Geometry *geometry, QPoint point)</arglist>
    </member>
    <member kind="signal">
      <type>void</type>
      <name>positionChanged</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>807f9cfb1b9d680ca76cf825cc9cf46a</anchor>
      <arglist>(Geometry *geom)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>addPoint</name>
      <anchorfile>classqmapcontrol_1_1LineString.html</anchorfile>
      <anchor>8694ab9a03b0ed4986c98ad727755f8a</anchor>
      <arglist>(Point *point)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual QRectF</type>
      <name>boundingBox</name>
      <anchorfile>classqmapcontrol_1_1LineString.html</anchorfile>
      <anchor>cbb256b5f9f888e9cd3bb475108ece24</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual QList&lt; Geometry * &gt;</type>
      <name>clickedPoints</name>
      <anchorfile>classqmapcontrol_1_1LineString.html</anchorfile>
      <anchor>c1cdf207019e92f289e62e9abb5ba2c3</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>Equals</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>029a8b50c439c719aac173bffe4cfb71</anchor>
      <arglist>(Geometry *geom)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>hasClickedPoints</name>
      <anchorfile>classqmapcontrol_1_1LineString.html</anchorfile>
      <anchor>3f7357f0362b6bee75f8c8c623fb528e</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual bool</type>
      <name>hasPoints</name>
      <anchorfile>classqmapcontrol_1_1LineString.html</anchorfile>
      <anchor>c3fc4ac8c80b5bf64c0bf095d7fde94b</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>isVisible</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>08422ee75ab02691943c1ca87e2bc563</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>LineString</name>
      <anchorfile>classqmapcontrol_1_1LineString.html</anchorfile>
      <anchor>d8efdad1cc0ff6c63357cb72180c3a0a</anchor>
      <arglist>(QList&lt; Point * &gt; const points, QString name=QString(), QPen *pen=0)</arglist>
    </member>
    <member kind="function">
      <type>QString</type>
      <name>name</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>2b0a198f837184bf6fff555cee3ce770</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>numberOfPoints</name>
      <anchorfile>classqmapcontrol_1_1LineString.html</anchorfile>
      <anchor>06b5ac0b597b8d1cb7e8817f7e66c2eb</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>Geometry *</type>
      <name>parentGeometry</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>771cc513dc079219d5da2c4b81019d7c</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>QPen *</type>
      <name>pen</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>ed7be2fcd2c1d7bccb55f5ac73d7a662</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>QList&lt; Point * &gt;</type>
      <name>points</name>
      <anchorfile>classqmapcontrol_1_1LineString.html</anchorfile>
      <anchor>18d4d26904bca7c54fb9d2a1b054c2fb</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>setName</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>6220fae15759fd0fa7d75e415df34e83</anchor>
      <arglist>(QString name)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>setPoints</name>
      <anchorfile>classqmapcontrol_1_1LineString.html</anchorfile>
      <anchor>6af8f478f54e6704e87dcf184a258a8c</anchor>
      <arglist>(QList&lt; Point * &gt; points)</arglist>
    </member>
    <member kind="function">
      <type>QString</type>
      <name>toString</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>3a013a6edb6d10a71297978bc31a796b</anchor>
      <arglist>()</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>qmapcontrol::MapAdapter</name>
    <filename>classqmapcontrol_1_1MapAdapter.html</filename>
    <member kind="function" virtualness="pure">
      <type>virtual QPoint</type>
      <name>coordinateToDisplay</name>
      <anchorfile>classqmapcontrol_1_1MapAdapter.html</anchorfile>
      <anchor>0a7f30d12395e615eec9440070795349</anchor>
      <arglist>(const QPointF &amp;coordinate) const =0</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>currentZoom</name>
      <anchorfile>classqmapcontrol_1_1MapAdapter.html</anchorfile>
      <anchor>8c7b803b9faa35db237e40c361e1c036</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="pure">
      <type>virtual QPointF</type>
      <name>displayToCoordinate</name>
      <anchorfile>classqmapcontrol_1_1MapAdapter.html</anchorfile>
      <anchor>a26c33260233907672b1b23f4b1fd033</anchor>
      <arglist>(const QPoint &amp;point) const =0</arglist>
    </member>
    <member kind="function">
      <type>QString</type>
      <name>host</name>
      <anchorfile>classqmapcontrol_1_1MapAdapter.html</anchorfile>
      <anchor>7ee8388c7880d8a3466967464f5034b6</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>maxZoom</name>
      <anchorfile>classqmapcontrol_1_1MapAdapter.html</anchorfile>
      <anchor>3a33e897bc474405772d17a7c81f8747</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>minZoom</name>
      <anchorfile>classqmapcontrol_1_1MapAdapter.html</anchorfile>
      <anchor>7457f51db57914a85bbefcc8c9fa97b4</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>tilesize</name>
      <anchorfile>classqmapcontrol_1_1MapAdapter.html</anchorfile>
      <anchor>f105c5a0cf588a3f60d67744bd353391</anchor>
      <arglist>() const </arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>qmapcontrol::MapControl</name>
    <filename>classqmapcontrol_1_1MapControl.html</filename>
    <member kind="enumeration">
      <name>MouseMode</name>
      <anchorfile>classqmapcontrol_1_1MapControl.html</anchorfile>
      <anchor>034ae8169a2202325de4ef39ffd3e431</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>Panning</name>
      <anchorfile>classqmapcontrol_1_1MapControl.html</anchorfile>
      <anchor>034ae8169a2202325de4ef39ffd3e431e105bcd8daf776fd01704a7186c49608</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>Dragging</name>
      <anchorfile>classqmapcontrol_1_1MapControl.html</anchorfile>
      <anchor>034ae8169a2202325de4ef39ffd3e431ea74c0c82481d6d724a43536424e3977</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>None</name>
      <anchorfile>classqmapcontrol_1_1MapControl.html</anchorfile>
      <anchor>034ae8169a2202325de4ef39ffd3e431c9d3e887722f2bc482bcca9d41c512af</anchor>
      <arglist></arglist>
    </member>
    <member kind="slot">
      <type>void</type>
      <name>resize</name>
      <anchorfile>classqmapcontrol_1_1MapControl.html</anchorfile>
      <anchor>148f8aec7ea97e2e465cf2bd979846ab</anchor>
      <arglist>(const QSize newSize)</arglist>
    </member>
    <member kind="slot">
      <type>void</type>
      <name>scroll</name>
      <anchorfile>classqmapcontrol_1_1MapControl.html</anchorfile>
      <anchor>527394cb8e8aa2d77f7a50a07b9e9f3e</anchor>
      <arglist>(const QPoint scroll)</arglist>
    </member>
    <member kind="slot">
      <type>void</type>
      <name>scrollDown</name>
      <anchorfile>classqmapcontrol_1_1MapControl.html</anchorfile>
      <anchor>51db121d79cb0a651a7441b98bb7d7a9</anchor>
      <arglist>(int pixel=10)</arglist>
    </member>
    <member kind="slot">
      <type>void</type>
      <name>scrollLeft</name>
      <anchorfile>classqmapcontrol_1_1MapControl.html</anchorfile>
      <anchor>02f3bf431288e7ed34ecc59f7b8de996</anchor>
      <arglist>(int pixel=10)</arglist>
    </member>
    <member kind="slot">
      <type>void</type>
      <name>scrollRight</name>
      <anchorfile>classqmapcontrol_1_1MapControl.html</anchorfile>
      <anchor>216e70011cb465e61e2992d761f568df</anchor>
      <arglist>(int pixel=10)</arglist>
    </member>
    <member kind="slot">
      <type>void</type>
      <name>scrollUp</name>
      <anchorfile>classqmapcontrol_1_1MapControl.html</anchorfile>
      <anchor>ed27d1373fd9e05fb86fa319df4fe375</anchor>
      <arglist>(int pixel=10)</arglist>
    </member>
    <member kind="slot">
      <type>void</type>
      <name>setZoom</name>
      <anchorfile>classqmapcontrol_1_1MapControl.html</anchorfile>
      <anchor>bb4bd8d8137d16816838c97d32407f39</anchor>
      <arglist>(int zoomlevel)</arglist>
    </member>
    <member kind="slot">
      <type>void</type>
      <name>updateRequest</name>
      <anchorfile>classqmapcontrol_1_1MapControl.html</anchorfile>
      <anchor>5cb68a198a28000fec8b7de1064d0a41</anchor>
      <arglist>(QRect rect)</arglist>
    </member>
    <member kind="slot">
      <type>void</type>
      <name>updateRequestNew</name>
      <anchorfile>classqmapcontrol_1_1MapControl.html</anchorfile>
      <anchor>cf37bc294477796509e00e8f546fbd44</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="slot">
      <type>void</type>
      <name>zoomIn</name>
      <anchorfile>classqmapcontrol_1_1MapControl.html</anchorfile>
      <anchor>7d7e315e34a66d9a66022d31635e7aca</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="slot">
      <type>void</type>
      <name>zoomOut</name>
      <anchorfile>classqmapcontrol_1_1MapControl.html</anchorfile>
      <anchor>72d29d38d8dd2c091cdd7078e1364f25</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="signal">
      <type>void</type>
      <name>boxDragged</name>
      <anchorfile>classqmapcontrol_1_1MapControl.html</anchorfile>
      <anchor>1fd82da86a16f8932d45f4a0cadb60d1</anchor>
      <arglist>(const QRectF)</arglist>
    </member>
    <member kind="signal">
      <type>void</type>
      <name>geometryClicked</name>
      <anchorfile>classqmapcontrol_1_1MapControl.html</anchorfile>
      <anchor>cc9ddd8e1721682682f85e73dae5f768</anchor>
      <arglist>(Geometry *geometry, QPoint coord_px)</arglist>
    </member>
    <member kind="signal">
      <type>void</type>
      <name>mouseEventCoordinate</name>
      <anchorfile>classqmapcontrol_1_1MapControl.html</anchorfile>
      <anchor>a463d9dfa43fa385b48aab115d374637</anchor>
      <arglist>(const QMouseEvent *evnt, const QPointF coordinate)</arglist>
    </member>
    <member kind="signal">
      <type>void</type>
      <name>viewChanged</name>
      <anchorfile>classqmapcontrol_1_1MapControl.html</anchorfile>
      <anchor>08eac7c36a9f2b940daa171d450f6d76</anchor>
      <arglist>(const QPointF &amp;coordinate, int zoom)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>addLayer</name>
      <anchorfile>classqmapcontrol_1_1MapControl.html</anchorfile>
      <anchor>87c6ef3c45ee9c21e173db2aa16cf567</anchor>
      <arglist>(Layer *layer)</arglist>
    </member>
    <member kind="function">
      <type>QPointF</type>
      <name>currentCoordinate</name>
      <anchorfile>classqmapcontrol_1_1MapControl.html</anchorfile>
      <anchor>010c83996ab94a3db104aecf0550d480</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>currentZoom</name>
      <anchorfile>classqmapcontrol_1_1MapControl.html</anchorfile>
      <anchor>8c7b803b9faa35db237e40c361e1c036</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>enablePersistentCache</name>
      <anchorfile>classqmapcontrol_1_1MapControl.html</anchorfile>
      <anchor>f27ac1bc11a80f7ff9a4bd3ec2527df7</anchor>
      <arglist>(const QDir &amp;path=QDir::homePath()+&quot;/QMapControl.cache&quot;)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>followGeometry</name>
      <anchorfile>classqmapcontrol_1_1MapControl.html</anchorfile>
      <anchor>6f7a69381097c74928af4ab3aa970386</anchor>
      <arglist>(const Geometry *geometry) const </arglist>
    </member>
    <member kind="function">
      <type>Layer *</type>
      <name>layer</name>
      <anchorfile>classqmapcontrol_1_1MapControl.html</anchorfile>
      <anchor>8e22ad201cb035fda7d7fefd8f348b11</anchor>
      <arglist>(const QString &amp;layername) const </arglist>
    </member>
    <member kind="function">
      <type>QList&lt; QString &gt;</type>
      <name>layers</name>
      <anchorfile>classqmapcontrol_1_1MapControl.html</anchorfile>
      <anchor>ee9467ec4674af32aed2fbeb1175e0b5</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>MapControl</name>
      <anchorfile>classqmapcontrol_1_1MapControl.html</anchorfile>
      <anchor>738341656ae7554bad78f0085c62c1f0</anchor>
      <arglist>(QSize size, MouseMode mousemode=Panning)</arglist>
    </member>
    <member kind="function">
      <type>MapControl::MouseMode</type>
      <name>mouseMode</name>
      <anchorfile>classqmapcontrol_1_1MapControl.html</anchorfile>
      <anchor>9106ab9dcac042b57fc4e797c94d84dc</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>moveTo</name>
      <anchorfile>classqmapcontrol_1_1MapControl.html</anchorfile>
      <anchor>f7e3575f01f98a4096ccf48ac6bd4a50</anchor>
      <arglist>(QPointF coordinate)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>numberOfLayers</name>
      <anchorfile>classqmapcontrol_1_1MapControl.html</anchorfile>
      <anchor>f3950823abbdf717124c279a386ca280</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>setMouseMode</name>
      <anchorfile>classqmapcontrol_1_1MapControl.html</anchorfile>
      <anchor>346037e336da8525fe41ec30bf216d82</anchor>
      <arglist>(MouseMode mousemode)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>setProxy</name>
      <anchorfile>classqmapcontrol_1_1MapControl.html</anchorfile>
      <anchor>ffaa4e5f0b461da0a1016d5459a550e0</anchor>
      <arglist>(QString host, int port)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>setView</name>
      <anchorfile>classqmapcontrol_1_1MapControl.html</anchorfile>
      <anchor>4ea85421ec8df905fba209510c909e2c</anchor>
      <arglist>(const Point *point) const </arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>setView</name>
      <anchorfile>classqmapcontrol_1_1MapControl.html</anchorfile>
      <anchor>c837e5df11959daca31bd1d01d12b94c</anchor>
      <arglist>(const QList&lt; QPointF &gt; coordinates) const </arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>setView</name>
      <anchorfile>classqmapcontrol_1_1MapControl.html</anchorfile>
      <anchor>b10ff3e8fed3a535de2a435ab1db48fb</anchor>
      <arglist>(const QPointF &amp;coordinate) const </arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>setViewAndZoomIn</name>
      <anchorfile>classqmapcontrol_1_1MapControl.html</anchorfile>
      <anchor>e8c52337a968729d53c1ba57bfd65ea4</anchor>
      <arglist>(const QList&lt; QPointF &gt; coordinates) const </arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>showScale</name>
      <anchorfile>classqmapcontrol_1_1MapControl.html</anchorfile>
      <anchor>a98cf43403323f601b3c91c6fe26a30c</anchor>
      <arglist>(bool show)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>stopFollowing</name>
      <anchorfile>classqmapcontrol_1_1MapControl.html</anchorfile>
      <anchor>f406ce597f3fa7ffe4e7d46c8b1e48c7</anchor>
      <arglist>(Geometry *geometry)</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>qmapcontrol::MapLayer</name>
    <filename>classqmapcontrol_1_1MapLayer.html</filename>
    <base>qmapcontrol::Layer</base>
    <member kind="enumeration">
      <name>LayerType</name>
      <anchorfile>classqmapcontrol_1_1Layer.html</anchorfile>
      <anchor>56943a0946e5f15e5e58054b8e7a04a4</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>MapLayer</name>
      <anchorfile>classqmapcontrol_1_1Layer.html</anchorfile>
      <anchor>56943a0946e5f15e5e58054b8e7a04a4fe7df421203e4175d260b8dabcbe3002</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>GeometryLayer</name>
      <anchorfile>classqmapcontrol_1_1Layer.html</anchorfile>
      <anchor>56943a0946e5f15e5e58054b8e7a04a46c04bd58c42df8a7539aba782503fee0</anchor>
      <arglist></arglist>
    </member>
    <member kind="slot">
      <type>void</type>
      <name>setVisible</name>
      <anchorfile>classqmapcontrol_1_1Layer.html</anchorfile>
      <anchor>18e44e30b31525a243960ca3928125aa</anchor>
      <arglist>(bool visible)</arglist>
    </member>
    <member kind="signal">
      <type>void</type>
      <name>geometryClicked</name>
      <anchorfile>classqmapcontrol_1_1Layer.html</anchorfile>
      <anchor>685dcab83356e5cc449475f177bb487d</anchor>
      <arglist>(Geometry *geometry, QPoint point)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>addGeometry</name>
      <anchorfile>classqmapcontrol_1_1Layer.html</anchorfile>
      <anchor>b692d7d08414ed2b744946b88872827f</anchor>
      <arglist>(Geometry *geometry)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>clearGeometries</name>
      <anchorfile>classqmapcontrol_1_1Layer.html</anchorfile>
      <anchor>0b551ef8f4d0fd26cfa2ed0ec4c981b3</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>isVisible</name>
      <anchorfile>classqmapcontrol_1_1Layer.html</anchorfile>
      <anchor>08422ee75ab02691943c1ca87e2bc563</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>QString</type>
      <name>layername</name>
      <anchorfile>classqmapcontrol_1_1Layer.html</anchorfile>
      <anchor>414e94fdd70490d75ddccb6923ae3410</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>Layer::LayerType</type>
      <name>layertype</name>
      <anchorfile>classqmapcontrol_1_1Layer.html</anchorfile>
      <anchor>1cfbd8a5c27cf9cb400fa458a1f70ba5</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>const MapAdapter *</type>
      <name>mapadapter</name>
      <anchorfile>classqmapcontrol_1_1Layer.html</anchorfile>
      <anchor>23cb6462136a0920739cdeb5f1709fa7</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>MapLayer</name>
      <anchorfile>classqmapcontrol_1_1MapLayer.html</anchorfile>
      <anchor>c50a4fcff04c6859ce5601543b8ac821</anchor>
      <arglist>(QString layername, MapAdapter *mapadapter, bool takeevents=true)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>removeGeometry</name>
      <anchorfile>classqmapcontrol_1_1Layer.html</anchorfile>
      <anchor>b34ffa138d7262b801cdfedaf4a129d3</anchor>
      <arglist>(Geometry *geometry)</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>qmapcontrol::OpenAerialMapAdapter</name>
    <filename>classqmapcontrol_1_1OpenAerialMapAdapter.html</filename>
    <base>qmapcontrol::TileMapAdapter</base>
    <member kind="function" virtualness="virtual">
      <type>virtual QPoint</type>
      <name>coordinateToDisplay</name>
      <anchorfile>classqmapcontrol_1_1TileMapAdapter.html</anchorfile>
      <anchor>94134b06e350d302f5b3a63f0016aa60</anchor>
      <arglist>(const QPointF &amp;) const </arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>currentZoom</name>
      <anchorfile>classqmapcontrol_1_1MapAdapter.html</anchorfile>
      <anchor>8c7b803b9faa35db237e40c361e1c036</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual QPointF</type>
      <name>displayToCoordinate</name>
      <anchorfile>classqmapcontrol_1_1TileMapAdapter.html</anchorfile>
      <anchor>601b4631d9d891eabffb063c510cc088</anchor>
      <arglist>(const QPoint &amp;) const </arglist>
    </member>
    <member kind="function">
      <type>QString</type>
      <name>host</name>
      <anchorfile>classqmapcontrol_1_1MapAdapter.html</anchorfile>
      <anchor>7ee8388c7880d8a3466967464f5034b6</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>maxZoom</name>
      <anchorfile>classqmapcontrol_1_1MapAdapter.html</anchorfile>
      <anchor>3a33e897bc474405772d17a7c81f8747</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>minZoom</name>
      <anchorfile>classqmapcontrol_1_1MapAdapter.html</anchorfile>
      <anchor>7457f51db57914a85bbefcc8c9fa97b4</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>OpenAerialMapAdapter</name>
      <anchorfile>classqmapcontrol_1_1OpenAerialMapAdapter.html</anchorfile>
      <anchor>8b9f8f912aa1bf34e1b4d11e7dfec394</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>tilesize</name>
      <anchorfile>classqmapcontrol_1_1MapAdapter.html</anchorfile>
      <anchor>f105c5a0cf588a3f60d67744bd353391</anchor>
      <arglist>() const </arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>qmapcontrol::OSMMapAdapter</name>
    <filename>classqmapcontrol_1_1OSMMapAdapter.html</filename>
    <base>qmapcontrol::TileMapAdapter</base>
    <member kind="function" virtualness="virtual">
      <type>virtual QPoint</type>
      <name>coordinateToDisplay</name>
      <anchorfile>classqmapcontrol_1_1TileMapAdapter.html</anchorfile>
      <anchor>94134b06e350d302f5b3a63f0016aa60</anchor>
      <arglist>(const QPointF &amp;) const </arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>currentZoom</name>
      <anchorfile>classqmapcontrol_1_1MapAdapter.html</anchorfile>
      <anchor>8c7b803b9faa35db237e40c361e1c036</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual QPointF</type>
      <name>displayToCoordinate</name>
      <anchorfile>classqmapcontrol_1_1TileMapAdapter.html</anchorfile>
      <anchor>601b4631d9d891eabffb063c510cc088</anchor>
      <arglist>(const QPoint &amp;) const </arglist>
    </member>
    <member kind="function">
      <type>QString</type>
      <name>host</name>
      <anchorfile>classqmapcontrol_1_1MapAdapter.html</anchorfile>
      <anchor>7ee8388c7880d8a3466967464f5034b6</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>maxZoom</name>
      <anchorfile>classqmapcontrol_1_1MapAdapter.html</anchorfile>
      <anchor>3a33e897bc474405772d17a7c81f8747</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>minZoom</name>
      <anchorfile>classqmapcontrol_1_1MapAdapter.html</anchorfile>
      <anchor>7457f51db57914a85bbefcc8c9fa97b4</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>OSMMapAdapter</name>
      <anchorfile>classqmapcontrol_1_1OSMMapAdapter.html</anchorfile>
      <anchor>b6f5dd078b96030385b14d2a5d777092</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>tilesize</name>
      <anchorfile>classqmapcontrol_1_1MapAdapter.html</anchorfile>
      <anchor>f105c5a0cf588a3f60d67744bd353391</anchor>
      <arglist>() const </arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>qmapcontrol::Point</name>
    <filename>classqmapcontrol_1_1Point.html</filename>
    <base>qmapcontrol::Geometry</base>
    <member kind="enumeration">
      <name>Alignment</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>cdfaca60ec19c0265bac2692d7982726</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TopLeft</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>cdfaca60ec19c0265bac2692d798272661f66ddc6702462a94d3e231f02b9017</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TopRight</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>cdfaca60ec19c0265bac2692d79827267e42a96f07eab63a8c9fa8a0526f34f4</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>BottomLeft</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>cdfaca60ec19c0265bac2692d7982726e61b9b6ea2fa75ca500d5bb1eaf6f6fc</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>BottomRight</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>cdfaca60ec19c0265bac2692d79827261640f649d644701a2f4633e6bd88b20c</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>Middle</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>cdfaca60ec19c0265bac2692d7982726673e6efef9aafe98078c5552e99c923c</anchor>
      <arglist></arglist>
    </member>
    <member kind="slot" virtualness="virtual">
      <type>virtual void</type>
      <name>setVisible</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>18e44e30b31525a243960ca3928125aa</anchor>
      <arglist>(bool visible)</arglist>
    </member>
    <member kind="signal">
      <type>void</type>
      <name>geometryClicked</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>685dcab83356e5cc449475f177bb487d</anchor>
      <arglist>(Geometry *geometry, QPoint point)</arglist>
    </member>
    <member kind="signal">
      <type>void</type>
      <name>positionChanged</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>807f9cfb1b9d680ca76cf825cc9cf46a</anchor>
      <arglist>(Geometry *geom)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual QRectF</type>
      <name>boundingBox</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>cbb256b5f9f888e9cd3bb475108ece24</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>QPointF</type>
      <name>coordinate</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>2fbb44b2ed047287d715484d2fda7299</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>Equals</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>029a8b50c439c719aac173bffe4cfb71</anchor>
      <arglist>(Geometry *geom)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>isVisible</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>08422ee75ab02691943c1ca87e2bc563</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>qreal</type>
      <name>latitude</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>6311aabecac471455760aae4790cff91</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>qreal</type>
      <name>longitude</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>2b0f7ec9068af09bcf151af61a785845</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>QString</type>
      <name>name</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>2b0a198f837184bf6fff555cee3ce770</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>Geometry *</type>
      <name>parentGeometry</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>771cc513dc079219d5da2c4b81019d7c</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>QPen *</type>
      <name>pen</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>ed7be2fcd2c1d7bccb55f5ac73d7a662</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>QPixmap *</type>
      <name>pixmap</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>166d4272301d85cc53e114ce4d26caf5</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Point</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>735b9dc17b1ba1595e7d29cadb4d18c1</anchor>
      <arglist>(qreal x, qreal y, QPixmap *pixmap, QString name=QString(), enum Alignment alignment=Middle)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Point</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>a1767675df0bc3c13c75b3a48241125e</anchor>
      <arglist>(qreal x, qreal y, QWidget *widget, QString name=QString(), enum Alignment alignment=Middle)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>Point</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>47257eee92b14e7c7f9b778c67bcb9a5</anchor>
      <arglist>(qreal x, qreal y, QString name=QString(), enum Alignment alignment=Middle)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>setBaselevel</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>91f1496833bfda9f7a7ec5fcb02a1895</anchor>
      <arglist>(int zoomlevel)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>setMaxsize</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>dc2724c4e195727b823ff55c940283de</anchor>
      <arglist>(QSize maxsize)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>setMinsize</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>c40b3e44f54fab1330b9309ac7bd84d2</anchor>
      <arglist>(QSize minsize)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>setName</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>6220fae15759fd0fa7d75e415df34e83</anchor>
      <arglist>(QString name)</arglist>
    </member>
    <member kind="function">
      <type>QString</type>
      <name>toString</name>
      <anchorfile>classqmapcontrol_1_1Geometry.html</anchorfile>
      <anchor>3a013a6edb6d10a71297978bc31a796b</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>QWidget *</type>
      <name>widget</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>d1eaabeb2b227cd055ccf4b4e2818480</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" protection="protected" virtualness="virtual">
      <type>virtual bool</type>
      <name>Touches</name>
      <anchorfile>classqmapcontrol_1_1Point.html</anchorfile>
      <anchor>73504541bf9e21ceceb82c6b4c3f2b04</anchor>
      <arglist>(Point *geom, const MapAdapter *mapadapter)</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>qmapcontrol::TileMapAdapter</name>
    <filename>classqmapcontrol_1_1TileMapAdapter.html</filename>
    <base>qmapcontrol::MapAdapter</base>
    <member kind="function" virtualness="virtual">
      <type>virtual QPoint</type>
      <name>coordinateToDisplay</name>
      <anchorfile>classqmapcontrol_1_1TileMapAdapter.html</anchorfile>
      <anchor>94134b06e350d302f5b3a63f0016aa60</anchor>
      <arglist>(const QPointF &amp;) const </arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>currentZoom</name>
      <anchorfile>classqmapcontrol_1_1MapAdapter.html</anchorfile>
      <anchor>8c7b803b9faa35db237e40c361e1c036</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual QPointF</type>
      <name>displayToCoordinate</name>
      <anchorfile>classqmapcontrol_1_1TileMapAdapter.html</anchorfile>
      <anchor>601b4631d9d891eabffb063c510cc088</anchor>
      <arglist>(const QPoint &amp;) const </arglist>
    </member>
    <member kind="function">
      <type>QString</type>
      <name>host</name>
      <anchorfile>classqmapcontrol_1_1MapAdapter.html</anchorfile>
      <anchor>7ee8388c7880d8a3466967464f5034b6</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>maxZoom</name>
      <anchorfile>classqmapcontrol_1_1MapAdapter.html</anchorfile>
      <anchor>3a33e897bc474405772d17a7c81f8747</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>minZoom</name>
      <anchorfile>classqmapcontrol_1_1MapAdapter.html</anchorfile>
      <anchor>7457f51db57914a85bbefcc8c9fa97b4</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>TileMapAdapter</name>
      <anchorfile>classqmapcontrol_1_1TileMapAdapter.html</anchorfile>
      <anchor>1233007f4393d3ae476a5284f9294e8c</anchor>
      <arglist>(const QString &amp;host, const QString &amp;serverPath, int tilesize, int minZoom=0, int maxZoom=17)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>tilesize</name>
      <anchorfile>classqmapcontrol_1_1MapAdapter.html</anchorfile>
      <anchor>f105c5a0cf588a3f60d67744bd353391</anchor>
      <arglist>() const </arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>qmapcontrol::WMSMapAdapter</name>
    <filename>classqmapcontrol_1_1WMSMapAdapter.html</filename>
    <base>qmapcontrol::MapAdapter</base>
    <member kind="function" virtualness="virtual">
      <type>virtual QPoint</type>
      <name>coordinateToDisplay</name>
      <anchorfile>classqmapcontrol_1_1WMSMapAdapter.html</anchorfile>
      <anchor>94134b06e350d302f5b3a63f0016aa60</anchor>
      <arglist>(const QPointF &amp;) const </arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>currentZoom</name>
      <anchorfile>classqmapcontrol_1_1MapAdapter.html</anchorfile>
      <anchor>8c7b803b9faa35db237e40c361e1c036</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual QPointF</type>
      <name>displayToCoordinate</name>
      <anchorfile>classqmapcontrol_1_1WMSMapAdapter.html</anchorfile>
      <anchor>601b4631d9d891eabffb063c510cc088</anchor>
      <arglist>(const QPoint &amp;) const </arglist>
    </member>
    <member kind="function">
      <type>QString</type>
      <name>host</name>
      <anchorfile>classqmapcontrol_1_1MapAdapter.html</anchorfile>
      <anchor>7ee8388c7880d8a3466967464f5034b6</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>maxZoom</name>
      <anchorfile>classqmapcontrol_1_1MapAdapter.html</anchorfile>
      <anchor>3a33e897bc474405772d17a7c81f8747</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>minZoom</name>
      <anchorfile>classqmapcontrol_1_1MapAdapter.html</anchorfile>
      <anchor>7457f51db57914a85bbefcc8c9fa97b4</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>tilesize</name>
      <anchorfile>classqmapcontrol_1_1MapAdapter.html</anchorfile>
      <anchor>f105c5a0cf588a3f60d67744bd353391</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>WMSMapAdapter</name>
      <anchorfile>classqmapcontrol_1_1WMSMapAdapter.html</anchorfile>
      <anchor>0bfce681fddbae34a9fae6fc8c2f0a38</anchor>
      <arglist>(QString host, QString serverPath, int tilesize=256)</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>qmapcontrol::YahooMapAdapter</name>
    <filename>classqmapcontrol_1_1YahooMapAdapter.html</filename>
    <base>qmapcontrol::TileMapAdapter</base>
    <member kind="function" virtualness="virtual">
      <type>virtual QPoint</type>
      <name>coordinateToDisplay</name>
      <anchorfile>classqmapcontrol_1_1TileMapAdapter.html</anchorfile>
      <anchor>94134b06e350d302f5b3a63f0016aa60</anchor>
      <arglist>(const QPointF &amp;) const </arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>currentZoom</name>
      <anchorfile>classqmapcontrol_1_1MapAdapter.html</anchorfile>
      <anchor>8c7b803b9faa35db237e40c361e1c036</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual QPointF</type>
      <name>displayToCoordinate</name>
      <anchorfile>classqmapcontrol_1_1TileMapAdapter.html</anchorfile>
      <anchor>601b4631d9d891eabffb063c510cc088</anchor>
      <arglist>(const QPoint &amp;) const </arglist>
    </member>
    <member kind="function">
      <type>QString</type>
      <name>host</name>
      <anchorfile>classqmapcontrol_1_1MapAdapter.html</anchorfile>
      <anchor>7ee8388c7880d8a3466967464f5034b6</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>maxZoom</name>
      <anchorfile>classqmapcontrol_1_1MapAdapter.html</anchorfile>
      <anchor>3a33e897bc474405772d17a7c81f8747</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>minZoom</name>
      <anchorfile>classqmapcontrol_1_1MapAdapter.html</anchorfile>
      <anchor>7457f51db57914a85bbefcc8c9fa97b4</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>tilesize</name>
      <anchorfile>classqmapcontrol_1_1MapAdapter.html</anchorfile>
      <anchor>f105c5a0cf588a3f60d67744bd353391</anchor>
      <arglist>() const </arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>YahooMapAdapter</name>
      <anchorfile>classqmapcontrol_1_1YahooMapAdapter.html</anchorfile>
      <anchor>9755050f27b44221d3e473b1cebd70cf</anchor>
      <arglist>()</arglist>
    </member>
  </compound>
</tagfile>
