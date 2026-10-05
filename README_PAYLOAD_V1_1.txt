ATLAS PAYLOAD MODE v1.1
=======================

Objetivo visual/funcional:
- Video/AI como vista principal.
- Mini-mapa operacional grande no canto inferior esquerdo.
- Rail de payload dedicado à esquerda.
- AI/Tracking/Gimbal/Event Log dedicados à direita.
- Telemetria MAVLink compacta, attitude/compass e system health na zona inferior.
- Toolbar ATLAS com JETSON/HM30/A8/VIDEO/AI.
- Sem depender de SSH na operação normal.

ALTERAÇÕES PRINCIPAIS
---------------------
1. Toolbar ATLAS aproximada ao mockup aprovado:
   ATLAS Ground Control | Payload Mode v1 | JETSON | HM30 | A8 | VIDEO | AI | settings.
2. Toolbar agora também existe como custom QML override, não apenas como alteração core.
3. QGC FlyViewWidgetLayer standard deixa de ser usado no Payload Mode, eliminando Takeoff/RTL,
   instrumentos e widgets QGC que estavam a sobrepor a interface ATLAS.
4. Mini-mapa maior e fixo, com botões Map/Targets, center e zoom.
5. Telemetria UAV em grelha 2x4, apenas com informação útil ao operador payload.
6. Attitude/Compass têm painel próprio.
7. System Status separado em Jetson/HM30/A8/Video/AI.
8. Right sidebar reorganizada: AI/Detection, Tracking, Gimbal e Event Log.
9. Gimbal panel tem indicador gráfico yaw/pitch.
10. Target card melhorado e target no mapa continua via CustomMapItems.qml.
11. Mock ganhou "demo start" / "demo stop" para simular a operação mostrada no conceito.

APLICAÇÃO
---------
A partir da raiz do repo:

  unzip -o ~/Downloads/ATLAS_Payload_Mode_v1_1.zip -d ~/ATLAS/ATLAS-QGroundControl

Como custom/atlas/CMakeLists.txt mudou, reconfigurar uma vez:

  PATH="$HOME/Qt/Tools/CMake/bin:$PATH" ~/Qt/6.11.1/gcc_64/bin/qt-cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

Depois:

  ~/Qt/Tools/CMake/bin/cmake --build build --parallel 2

TESTE SEM HARDWARE
------------------
Terminal 1:

  python3 tools/atlas/mock_jetson_server.py

Terminal 2:

  ./build/Release/ATLAS-QGroundControl

No mock:

  demo start

Isto coloca HM30/A8 online, inicia REC simulado, ativa AI/SAHI/Depth, cria target #3,
ativa tracking/auto-gimbal e faz target/gimbal variar ao longo do tempo.

Outros testes:
  demo stop
  all on
  all off
  target on
  target off
  link bad
  link good
  hot on
  hot off
  freeze on
  freeze off

NOTAS
-----
- VIDEO só fica online quando o decoder REAL do QGC estiver a receber frames.
- A telemetria MAVLink aparece quando existir Vehicle real/SITL.
- Bounding boxes continuarão a vir do stream processado pela Jetson; o QGC gere a interface,
  seleção/tracking e os estados de payload.
- O tuning GStreamer LOW/BALANCED/ROBUST deve ser feito com o HM30 real.
