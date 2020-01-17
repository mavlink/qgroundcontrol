/* GStreamer CPU Report Element
 * Copyright (C) <2010> Zaheer Abbas Merali <zaheerabbas merali org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __GST_CPU_REPORT_H__
#define __GST_CPU_REPORT_H__

#include <time.h>

#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS
#define GST_TYPE_CPU_REPORT \
  (gst_cpu_report_get_type())
#define GST_CPU_REPORT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_CPU_REPORT,GstCpuReport))
#define GST_CPU_REPORT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_CPU_REPORT,GstCpuReportClass))
#define GST_IS_CPU_REPORT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_CPU_REPORT))
#define GST_IS_CPU_REPORT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_CPU_REPORT))
typedef struct _GstCpuReport GstCpuReport;
typedef struct _GstCpuReportClass GstCpuReportClass;

struct _GstCpuReport
{
  GstBaseTransform basetransform;

  GstClockTime start_time;
  GstClockTime last_time;
  clock_t last_cpu_time;
};

struct _GstCpuReportClass
{
  GstBaseTransformClass parent_class;
};

GType gst_cpu_report_get_type (void);

G_END_DECLS
#endif /* __GST_CPU_REPORT_H__ */
