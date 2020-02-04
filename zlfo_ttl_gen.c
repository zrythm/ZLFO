/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of ZLFO
 *
 * ZLFO is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * ZLFO is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU General Affero Public License
 * along with ZLFO.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdio.h>

#include "zlfo_common.h"

typedef enum PortType
{
  PORT_TYPE_FLOAT,
  PORT_TYPE_INT,
  PORT_TYPE_TOGGLE,
} PortType;

typedef enum NodeProperty
{
  NODE_PROP_POS,
  NODE_PROP_VAL,
  NODE_PROP_CURVE,
} NodeProperty;

int main (
  int argc, const char* argv[])
{
  if (argc != 2)
    {
      fprintf (
        stderr,
        "Need 1 argument, received %d\n", argc - 1);
      return -1;
    }

  FILE * f = fopen (argv[1], "w");
  if (!f)
    {
      fprintf (
        stderr, "Failed to open file %s\n", argv[1]);
      return -1;
    }

  fprintf (f,
"@prefix atom: <http://lv2plug.in/ns/ext/atom#> .\n\
@prefix doap: <http://usefulinc.com/ns/doap#> .\n\
@prefix lv2:  <http://lv2plug.in/ns/lv2core#> .\n\
@prefix midi: <http://lv2plug.in/ns/ext/midi#> .\n\
@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .\n\
@prefix foaf: <http://xmlns.com/foaf/0.1/> .\n\
@prefix urid: <http://lv2plug.in/ns/ext/urid#> .\n\
@prefix ui:   <http://lv2plug.in/ns/extensions/ui#> .\n\
@prefix log:  <http://lv2plug.in/ns/ext/log#> .\n\n");

  fprintf (f,
"<" LFO_URI ">\n\
  a lv2:Plugin,\n\
    lv2:OscillatorPlugin ;\n\
  doap:name \"ZLFO\" ;\n\
  doap:maintainer [\n\
    foaf:name \"\"\"Alexandros Theodotou\"\"\" ;\n\
    foaf:homepage <https://www.zrythm.org> ;\n\
  ] ;\n\
  doap:license <https://www.gnu.org/licenses/agpl-3.0.html> ;\n\
  lv2:project <" PROJECT_URI "> ;\n\
  lv2:requiredFeature urid:map ;\n\
  lv2:optionalFeature lv2:hardRTCapable ;\n\
  lv2:optionalFeature log:log ;\n\
  lv2:port [\n\
    a lv2:InputPort ,\n\
      atom:AtomPort ;\n\
    atom:bufferType atom:Sequence ;\n\
    lv2:index 0 ;\n\
    lv2:designation lv2:control ;\n\
    lv2:symbol \"control\" ;\n\
    lv2:name \"Control\" ;\n\
    rdfs:comment \"GUI to plugin communication\" ;\n\
  ] , [\n\
    a lv2:OutputPort ,\n\
      atom:AtomPort ;\n\
    atom:bufferType atom:Sequence ;\n\
    lv2:index 1 ;\n\
    lv2:designation lv2:control ;\n\
    lv2:symbol \"notify\" ;\n\
    lv2:name \"Notify\" ;\n\
    rdfs:comment \"Plugin to GUI communication\" ;\n\
  ] , [\n");

  /* write input controls */
  int index = ZLFO_GATE;
  for (int i = index; i <= ZLFO_NUM_NODES; i++)
    {
      float def = 0.f;
      float min = 0.f;
      float max = 1.f;
      int defi = 0;
      int mini = 0;
      int maxi = 1;
      int is_trigger = 0;
      PortType type = PORT_TYPE_FLOAT;
      char symbol[256] = "\0";
      char name[256] = "\0";
      char comment[800] = "\0";
      switch (i)
        {
        case ZLFO_GATE:
          strcpy (symbol, "gate");
          strcpy (name, "Gate");
          strcpy (
            comment, "Not used at the moment");
          break;
        case ZLFO_TRIGGER:
          strcpy (symbol, "trigger");
          strcpy (name, "Trigger");
          is_trigger = 1;
          break;
        case ZLFO_FREQ:
          strcpy (symbol, "freq");
          strcpy (name, "Frequency");
          strcpy (
            comment, "Frequency if free running");
          def = 1.f;
          max = 20.f;
          break;
        case ZLFO_SHIFT:
          strcpy (symbol, "shift");
          strcpy (name, "Shift");
          strcpy (comment, "Shift (phase)");
          def = 0.5f;
          break;
        case ZLFO_RANGE_MIN:
          strcpy (symbol, "range_min");
          strcpy (name, "Range min");
          min = -1.f;
          def = -1.f;
          break;
        case ZLFO_RANGE_MAX:
          strcpy (symbol, "range_max");
          strcpy (name, "Range max");
          min = -1.f;
          def = 1.f;
          break;
        case ZLFO_STEP_MODE:
          strcpy (symbol, "step_mode");
          strcpy (name, "Step mode");
          strcpy (comment, "Step mode enabled");
          type = PORT_TYPE_TOGGLE;
          break;
        case ZLFO_FREE_RUNNING:
          strcpy (symbol, "free_running");
          strcpy (name, "Free running");
          strcpy (comment, "Free run toggle");
          type = PORT_TYPE_TOGGLE;
          def = 1.f;
          break;
        case ZLFO_NUM_NODES:
          strcpy (symbol, "num_nodes");
          strcpy (name, "Node count");
          type = PORT_TYPE_INT;
          defi = 2;
          mini = 2;
          maxi = 16;
          break;
        default:
          break;
        }
      if (i >= ZLFO_NODE_1_POS &&
          i <= ZLFO_NODE_16_VAL)
        {
          NodeProperty prop =
            (i - ZLFO_NODE_1_POS) % 3;
          int node_id = (i - ZLFO_NODE_1_POS) / 3 + 1;

          switch (prop)
            {
            case NODE_PROP_POS:
              sprintf (
                symbol, "node_%d_pos", node_id);
              sprintf (
                name, "Node %d position", node_id);
              if (node_id == 2)
                def = 1.f;
              break;
            case NODE_PROP_VAL:
              sprintf (
                symbol, "node_%d_val", node_id);
              sprintf (
                name, "Node %d value", node_id);
              if (node_id == 1)
                def = 1.f;
              break;
            case NODE_PROP_CURVE:
              sprintf (
                symbol, "node_%d_curve", node_id);
              sprintf (
                name, "Node %d curve", node_id);
              break;
            default:
              break;
            }
        }

      /* write port */
      fprintf (f,
"    a lv2:InputPort ,\n\
      lv2:ControlPort ;\n\
    lv2:index %d ;\n\
    lv2:symbol \"%s\" ;\n\
    lv2:name \"%s\" ;\n",
        i, symbol, name);

      if (comment[0] != '\0')
        {
          fprintf (f,
"    rdfs:comment \"%s\" ;\n",
            comment);
        }

      if (type == PORT_TYPE_FLOAT ||
          type == PORT_TYPE_TOGGLE)
        {
          fprintf (f,
"    lv2:default %f ;\n\
    lv2:minimum %f ;\n\
    lv2:maximum %f ;\n",
            (double) def, (double) min, (double) max);
        }
      else if (type == PORT_TYPE_INT)
        {
          fprintf (f,
"    lv2:default %d ;\n\
    lv2:minimum %d ;\n\
    lv2:maximum %d ;\n",
            defi, mini, maxi);
        }

      if (is_trigger)
        {
          fprintf (f,
"    lv2:portProperty lv2:trigger ;\n");
        }
      else if (type == PORT_TYPE_INT)
        {
          fprintf (f,
"    lv2:portProperty lv2:integer ;\n");
        }
      else if (type == PORT_TYPE_TOGGLE)
        {
          fprintf (f,
"    lv2:portProperty lv2:toggled ;\n");
        }

      fprintf (f,
"  ] , [\n");
    }

  /* write cv out and audio out */
  fprintf (f,
"    a lv2:OutputPort ,\n\
      lv2:CVPort ;\n\
    lv2:index %d ;\n\
    lv2:symbol \"cv_out\" ;\n\
    lv2:name \"CV output\" ;\n\
  ] , [\n\
    a lv2:OutputPort ,\n\
      lv2:AudioPort ;\n\
    lv2:index %d ;\n\
    lv2:symbol \"audio_out\" ;\n\
    lv2:name \"Audio output\" ;\n\
  ] .\n\n", ZLFO_CV_OUT, ZLFO_AUDIO_OUT);

  /* write UI */
  fprintf (f,
"<" LFO_UI_URI ">\n\
  a ui:X11UI ;\n\
  lv2:requiredFeature urid:map ;\n\
  lv2:optionalFeature log:log ;\n\
  lv2:optionalFeature ui:noUserResize ;\n\
  lv2:extensionData ui:idleInterface ;\n\
  ui:portNotification [\n\
    ui:plugin \"" LFO_URI "\" ;\n\
    lv2:symbol \"notify\" ;\n\
    ui:notifyType atom:Blank ;\n\
  ] .");

  fclose (f);

  return 0;
}
