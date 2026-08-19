Да, теперь картина становится намного яснее. И я согласен с твоей основной мыслью: мы начинаем упираться не просто в «рендеринг», а в границу между visual state и animation system.

Но сначала я хочу поправить одну вещь, потому что source сейчас говорит немного не то, что ты описал по PositionMode.

1. PositionMode::Absolute: сейчас source задаёт его относительно родителя, а не экрана

В types.hpp действительно есть:

enum class PositionMode
{
    Layout,
    Absolute
};

Но фактический arrangeRecursive() сейчас делает для absolute-child:

child->actualPosition_ = ctx.contentPosition + child->position_;

То есть position_ добавляется к content position родителя. Это не screen-space позиционирование.

И linear_layout.cpp подтверждает, что Absolute означает прежде всего выход из normal linear flow: такой child не участвует в обычной последовательной раскладке, а потом отдельно получает geometry.

Поэтому здесь есть важное архитектурное различие:

Absolute
=
не участвовать в normal flow

и отдельно:

Absolute
=
позиционироваться относительно screen

Сейчас первое реализовано точно. Второе — нет.

Это хороший пример того, почему Phase 4 стоит начинать только после того, как мы чётко определим coordinate spaces. Я бы пока не менял это, потому что это уже пересекается с Phase 2 semantics.

2. И вот это очень важно: actualPosition у нас уже действительно final geometry

Здесь твоя память в целом правильная.

actualPosition_ и actualSize_ назначаются во время arrangeRecursive(), после вычисления финального размера:

child->actualPosition_ = position;
child->actualSize_ = internal::resolveFinalSize(...);

То есть концептуально:

requested position
       ↓
layout rules
       ↓
measure
       ↓
arrange
       ↓
actualPosition / actualSize

И это очень хорошая граница.

Я бы вообще закрепил терминологически:

position_
    = requested/layout input


desiredSize_
    = measurement result


actualPosition_
    = final arranged geometry


actualSize_
    = final arranged geometry

Это поможет нам не смешивать layout intent и render geometry.

3. А теперь самое интересное: animation действительно появляется здесь

Я думаю, ты очень точно заметил закономерность:

opacity
transform
fade
scale
position interpolation

сами по себе не являются animation system.

Они являются visual properties, которые animation system может менять во времени.

Например:

opacity = 0.4

— это rendering state.

А:

opacity:
0.0 → 1.0 over 250 ms

— это animation.

То же самое с transform:

scale = 0.8

— visual state.

scale:
0.8 → 1.0

— animation.

Поэтому я бы сейчас очень чётко разделил эти два уровня.

Rendering layer
Node
 ├── opacity
 ├── transform
 ├── visual state
 └── draw()
Animation layer
Animation
    ↓
changes Node visual properties over time

И это совсем не обязательно значит, что нам нужен огромный AnimationManager.

4. Legacy Modal как раз показывает, почему этот вопрос появился

Я посмотрел старую src/components/modal.cpp.

Там действительно уже есть transition-like поведение:

transitionProgress
transitionAlpha
transitionTime
deltaTime

и в onUpdate() альфа постепенно изменяется.

Причём это делается не через отдельное opacity свойство, а через изменение alpha самого background color:

a = std::min(maxBackdropAlpha + transitionAlpha, maxAlpha);
setBackgroundColor({r, g, b, a});

Это важное наблюдение:

В legacy code уже существовала потребность в temporal visual state, но она была реализована локально внутри компонента.

И это именно тот случай, когда архитектура нового framework должна решить:

является ли fade ответственностью компонента?
или
есть ли framework-level mechanism для animation?

При этом legacy component сам по себе не является основанием новой архитектуры — это только свидетельство реальной потребности приложения. ARCHITECTURE и PHASE4_START_CONTEXT это отдельно оговаривают.

5. По поводу твоей идеи «layout invalidation уже достаточно»

Вот здесь я бы немного возразил.

Ты очень правильно заметил:

layout invalidation

у нас уже достаточно широкая и root-based.

Но:

layout invalidation и draw invalidation — это разные семантики, даже если первый сейчас может фактически приводить ко второму.

Например, представим:

Node.opacity = 0.5

Layout от этого не меняется.

Но rendering должен измениться.

Если мы скажем:

каждое visual изменение
→ layout invalidation root
→ measure
→ arrange
→ draw

то это будет работать функционально, но архитектурно получится, что rendering property заставляет нас запускать layout pipeline.

То же самое:

background color
border color
opacity
transform animation

Не обязательно должны инвалидировать layout.

И особенно:

60 FPS animation

Если opacity меняется каждый кадр, делать:

measure
arrange
draw

каждый кадр — возможно допустимо для маленького UI, но это уже плохая архитектурная связь.

Поэтому я бы сформулировал так:

Layout invalidation
    → geometry may have changed


Visual invalidation
    → pixels may have changed


Animation/update
    → visual and/or layout state may change
6. Но отдельный DrawInvalidation прямо сейчас я бы всё равно не создавал

И вот здесь я скорее соглашусь с тобой.

Нам не обязательно сейчас добавлять:

drawInvalidation_
drawQueue_
dirtyRegions_

Я бы сначала разделил семантику и механизм.

То есть определить:

layout-affecting change
    → layout invalidation


visual-only change
    → no layout invalidation

Но сам rendering всё равно может сегодня выполнять:

draw whole tree every frame

Это совершенно нормальный первый этап.

Тогда получается:

Phase 4


layout invalidation
     │
     └── geometry invalidation


visual changes
     │
     └── current frame sees them automatically


draw
     │
     └── redraw tree each frame

И это очень чистая архитектура.

А потом, если понадобится optimization:

visual invalidation
      ↓
dirty region
      ↓
partial redraw

можно будет добавить позже без того, чтобы semantic model пришлось переделывать.

Это, на мой взгляд, лучшее решение на данный момент.

7. И это автоматически подводит нас к Animation

Я бы не создавал отдельный animation layer прямо сейчас.

Я бы сначала зафиксировал что animation layer должен делать, если он появится.

Например:

Node visual properties
────────────────────────
opacity
transform
position?
color?

Animation system:

Animation
   target
   start value
   end value
   duration
   timing/easing

И каждый frame:

UIManager::runFrame(dt)
        ↓
Animation/update
        ↓
Node visual state
        ↓
draw

То есть animation system не должен рисовать.

Он только меняет state.

Это очень важно.

8. Тогда возникает важное архитектурное решение: где живут animation objects?

Я вижу три возможных модели.

Вариант A — component-owned
Modal
 └── FadeAnimation

Плюс: просто.

Минус: каждый component начинает изобретать собственную animation machinery.

Вариант B — Node-owned
Node
 ├── opacity
 ├── transform
 └── animations

Плюс: удобно.

Минус: Node начинает превращаться в очень тяжёлый объект:

runtime
layout
input
events
render
animation

А мы уже пытаемся не допускать такого разрастания.

Вариант C — отдельный Animation subsystem
UIManager
   │
   ├── NodeTree
   ├── InputManager
   ├── LayoutManager
   └── AnimationManager

AnimationManager работает с Node state:

AnimationManager
       ↓
   Node visual properties
       ↓
      draw

Архитектурно это выглядит наиболее чисто.

Но пока у нас есть только один реальный сценарий — fade modal — я бы не вводил его до появления ещё хотя бы нескольких независимых animation requirements.

9. Поэтому я бы сейчас не включал «Animation System» в Phase 4

Зато я бы добавил в Phase 4 одну очень важную вещь:

определить Visual State Contract

То есть ответить:

Что может влиять на то, что Node рисует?

Я бы начал минимум с:

visibility
opacity
geometry
clipping
possibly transform
style/visual properties

А потом уже:

Animation
    = механизм изменения этих properties over time

Это очень хороший separation of concerns.

10. С transform я тоже стал осторожнее

После того как мы посмотрели на actual geometry, я бы не бросался добавлять matrix transform прямо сейчас.

Потому что нам сначала нужно точно решить:

actualPosition
    ↓
local space?
parent content space?
viewport space?
screen space?

И PositionMode::Absolute нам в этом как раз помогает увидеть проблему.

Если мы действительно захотим:

PositionMode::Absolute

как viewport/screen positioning, это должно быть выражено layout contract.

А вот:

transform

должен быть уже следующим визуальным преобразованием:

layout geometry
      ↓
visual transform
      ↓
render coordinates

Тогда очень естественная цепочка:

Layout
  ↓
actual geometry
  ↓
Transform
  ↓
Clipping
  ↓
Opacity / composition
  ↓
Node::draw()

Вот это уже очень хорошая архитектурная модель.

11. Я бы теперь расширил Phase 4 примерно так

Не добавляя пока Animation Manager:

Phase 4 — Rendering / Backend


A. Geometry → Rendering boundary
   - meaning of actualPosition / actualSize
   - coordinate spaces
   - logical vs render coordinates


B. Render traversal
   - root / overlay / modal ordering
   - subtree traversal
   - mutation safety


C. Renderer state
   - state isolation
   - nested clipping


D. Visual state
   - visibility
   - opacity
   - colors/styles
   - future transform boundary


E. Backend
   - SDL3 contract
   - no premature abstraction


F. Resources
   - define ownership boundary
   - no ResourceManager yet


G. Animation boundary
   - do not implement system yet
   - define that animation modifies visual/layout state

Вот G я считаю особенно полезным.

Не Animation System.

А Animation Boundary.

12. И тут у меня есть несколько действительно важных вопросов к тебе

Не столько для уточнения кода, сколько для формирования архитектуры.

Первый

Ты хочешь, чтобы animation в будущем могла менять только visual properties:

opacity
transform
color

или также layout properties:

position
size
padding

Потому что это два очень разных сценария.

Если animation может менять size и position, она становится связана с LayoutManager.

Если только visual state — она может жить значительно проще:

Animation
    ↓
visual state
    ↓
render
Второй

Нужно ли нам вообще сейчас иметь transform как property, или можно оставить это будущим extension point?

Я бы сейчас склонялся к:

translation — потенциально полезна
scale/rotation — пока не обязательны

но важно не закрыть их архитектурно.

Третий

И самое интересное:

хочешь ли ты, чтобы framework в будущем поддерживал animation как встроенную концепцию вообще?

Не «нужна ли нам fade сейчас», а именно:

Node
   ↑
Animation
   ↑
Timeline / easing

или мы хотим принцип:

Framework предоставляет visual state, а animation полностью является responsibility клиента/component layer.

Оба варианта жизнеспособны, но это уже очень серьёзное архитектурное решение.

А вот PositionMode::Absolute я бы пока отдельно зафиксировал как несовпадение между текущей памятью/ожиданием и фактическим source: сейчас это absolute-from-parent-content, а не from-screen. Это важно не смешать с Phase 4 и случайно не переделать layout semantics ради rendering.

И я бы пока вообще ничего нового не коммитил. Сначала стоит решить эти границы: visual state → animation → layout. Именно здесь сейчас появляется настоящий архитектурный вопрос Phase 4.
------------------------------------------
Отлично. Проверил phase4-rendering: твоя правка применена, и теперь SDL_GetRenderClipRect() используется с правильной SDL3 bool-семантикой.

C теперь закрыт

Фактический rendering-state contract:

RAII-изоляция renderer state — есть.
Сохраняются/restored target, viewport, clip, scale, draw color, blend mode, color scale.
toSDLRect() строится из actualPosition/actualSize.
Nested clipping вычисляется через пересечение родительского clip с bounds текущего node.
SDL_GetRenderClipRect() теперь используется без ошибочного == 0.
Изменения не требуют API RenderContext или другого backend layer.

Таким образом:

C. Renderer state


state isolation          ✅
state restoration        ✅
nested clipping          ✅
SDL3 clip semantics      ✅

Я также сверил актуальный branch и убедился, что nodetree.cpp после твоего patch действительно находится в нужном состоянии. Compilation/tests/runtime всё ещё не запускались.

Теперь можем спокойно перейти к E — Backend, но именно как к обсуждению архитектурной границы, а не как к обязательному коду.

в документах где-то описано откуда поставляется и где находится сам sdl3 backend?

Нет, насколько я вижу, в текущей документации это не описано.

Я проверил прежде всего:

docs/ARCHITECTURE.md — описывает SDL3 как текущую rendering technology/backend boundary, но не источник поставки, установку или конкретное место расположения SDL3.
docs/ROADMAP.md — задаёт Phase 4 как Rendering / Backend, но также не говорит, откуда берётся SDL3.
README.md — лишь говорит, что framework построен на SDL3.
docs/INSTRUCTIONS.md — содержит общие правила разработки, но ничего о dependency provisioning.

При этом в source действительно используется SDL3:

#include <SDL3/SDL.h>

но из просмотренных файлов не следует, каким способом dependency должна быть получена: system package, vendored dependency, CMake FetchContent, submodule, package manager и т. п.

Здесь я бы не стал угадывать. Поскольку CMake/build configuration сейчас не удалось обнаружить по ожидаемому CMakeLists.txt, следующий полезный шаг — посмотреть корень и build/dependency-файлы ветки и установить фактический механизм поставки SDL3.

И уже после этого мы сможем нормально обсудить E:

что именно мы считаем “SDL3 backend” в архитектуре framework — сам внешний SDL3 dependency или ещё наш собственный слой интеграции с ним?

Это как раз важное различие для Phase 4.
---------------------------------------------
сейчас идея с sdl следующая: сам sdl3 как зависимость находится в ui фреймворке и поставляется самим фреймворком, то есть пользователю не нужно отдельно скачивать себе sdl3, он поставляется вместе с фреймворком, но при этом за жизненный цикл отвечает разработчик, а не фреймворк, фреймворк поставляет лишь sdl3 и получает нужный ему контекст sdl. cmakelists на данный момент находится у меня локально в vscode, так как сборкой я не занимаюсь на текущий момент, но в репозитории его нет и он там не нужен, но сам файл выглядит примерно так пока что.
cmake_minimum_required(VERSION 3.20) 
 
project(ui_framework 
    VERSION 1.0 
    LANGUAGES CXX 
) 
 
set(CMAKE_CXX_STANDARD 20) 
set(CMAKE_CXX_STANDARD_REQUIRED ON) 
 
# ========================== 
# External dependencies 
# ========================== 
 
list(APPEND CMAKE_PREFIX_PATH 
    ${CMAKE_CURRENT_SOURCE_DIR}/external/SDL3 
    ${CMAKE_CURRENT_SOURCE_DIR}/external/SDL3_image 
    ${CMAKE_CURRENT_SOURCE_DIR}/external/SDL3_ttf 
) 
 
find_package(SDL3 REQUIRED) 
find_package(SDL3_image REQUIRED) 
find_package(SDL3_ttf REQUIRED) 
 
 
# ========================== 
# UI Framework library 
# ========================== 
 
add_library(ui_framework STATIC 
    src/core/ui_manager.cpp 
    src/core/nodetree.cpp 
    src/core/node.cpp 
    src/core/panelnode.cpp 
    src/core/stackpanelnode.cpp 
    src/core/layoutmanager.cpp 
    src/core/modalmanager.cpp 
    src/core/inputmanager.cpp 
    src/core/primitives.cpp 
) 
 
 
# ========================== 
# Include directories 
# ========================== 
 
target_include_directories(ui_framework 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/include 
) 
 
target_compile_features(ui_framework PUBLIC cxx_std_20) 
 
# ========================== 
# Link dependencies 
# ========================== 
 
target_link_libraries(ui_framework 
    PUBLIC 
        SDL3::SDL3 
        SDL3_image::SDL3_image 
        SDL3_ttf::SDL3_ttf 
)
--------------------
также следующим сообщением пришлю скриншот  того, где и в каком виде у меня находится sdl3.

Понял. Тогда модель E у нас существенно яснее, и она отличается от предположения, которое я делал раньше.

Текущий замысел такой:

                    UI Framework
                         │
            поставляет вместе с собой
                         │
              ┌──────────▼──────────┐
              │        SDL3         │
              │   SDL3_image/ttf    │
              └──────────┬──────────┘
                         │
                         │ framework использует
                         ▼
                  SDL renderer/context
                         │
                         ▼
                    UI Framework

При этом:

SDL3 delivery / availability
        → ответственность framework


SDL3 lifetime / initialization / shutdown
        → ответственность application developer

И это вполне разумная boundary.

То есть framework не владеет lifecycle SDL, но гарантирует, что нужная SDL3 dependency поставляется вместе с framework. Пользователь получает framework и не должен отдельно искать/скачивать SDL3.

Твой локальный CMake это подтверждает: dependency ожидается внутри external/SDL3, external/SDL3_image, external/SDL3_ttf, а ui_framework линкуется против соответствующих SDL targets.

При этом я бы пока не стал вносить это в документацию или код, пока не увижу скриншот и фактическую структуру external/. Важен вопрос, что именно мы поставляем: исходники SDL, уже подготовленный пакет, binaries, headers + libraries, какую структуру target'ов предполагаем и т. д.

После скриншота я смогу точно сформулировать E как архитектурный контракт, включая:

1. Что framework поставляет
2. Что framework инициализирует / не инициализирует
3. Что framework принимает от application
4. Кому принадлежит SDL_Window
5. Кому принадлежит SDL_Renderer
6. Какие SDL extensions входят в delivery
7. Где заканчивается ответственность framework

Присылай скриншот — сначала разберём именно эту границу, без перехода к B.
----------------------------------
Дальше я хочу обсудить уже непосредственно сам подход и как лучше двигаться архитектурно с render бекендом

Да, теперь модель E стала достаточно конкретной. По скриншоту видно, что external/ действительно является частью дерева ui_framework и содержит готовые SDL3 / SDL3_image / SDL3_ttf distributions: bin, include, lib, share. То есть это не просто «мы ожидаем, что пользователь установит SDL», а именно framework delivery model.

И здесь я бы разделил два понятия, которые сейчас легко смешать:

E. Backend
│
├── 1. Dependency delivery
│      Кто поставляет SDL3?
│
├── 2. SDL ownership/lifecycle
│      Кто создаёт/уничтожает SDL state?
│
└── 3. Rendering backend abstraction
       Насколько framework абстрагирует сам renderer?

Первые два у нас уже фактически решены. Третий — как раз тот вопрос, который стоит сейчас обсудить.

1. Что мы уже точно решили

Я бы зафиксировал текущую модель так:

Application
    │
    │ owns
    ▼
SDL lifecycle
    │
    ├── SDL_Init(...)
    ├── SDL_Window
    ├── SDL_Renderer
    └── SDL_Quit(...)
              │
              │ passed into framework
              ▼
       UIManager / NodeTree
              │
              ▼
             SDL3

Framework при этом:

поставляет SDL3;
поставляет SDL3_image / SDL3_ttf;
использует их API;
получает SDL_Renderer* от application;
не создаёт и не уничтожает renderer;
не вызывает общий SDL lifecycle самостоятельно.

Это очень хорошая граница.

Она означает:

SDL является infrastructure dependency framework, но SDL runtime state остаётся во владении application.

Это сильно лучше, чем заставлять framework создавать окно/renderer, потому что пользователь может хотеть:

SDL window
+ game renderer
+ UI framework

в одном процессе.

2. Тогда главный вопрос: нужен ли нам вообще «render backend abstraction»?

Я сейчас вижу три возможных уровня.

Вариант 1 — текущий SDL-first
NodeTree
   ↓
SDL_Renderer*
   ↓
SDL3

Node::draw() напрямую знает SDL.

Это фактически текущая модель.

Преимущество — очень мало кода и абсолютно прозрачный rendering path.

Вариант 2 — небольшой framework rendering layer

Например:

NodeTree
   ↓
Framework rendering helpers
   ↓
SDL3

Где framework абстрагирует только свои гарантии:

clip
renderer state
coordinate conversion
basic primitives

Но node всё ещё в конечном итоге рисует через SDL.

Это примерно то, куда мы сейчас уже движемся благодаря RendererStateScope.

Вариант 3 — настоящий backend abstraction

Например:

NodeTree
      ↓
RenderContext
      ↓
IRenderBackend
   ↙        ↘
SDL3       Vulkan/OpenGL/...

Вот это я сейчас не вижу необходимости делать.

И более того, я бы сопротивлялся этому в Phase 4.

3. Почему я пока за вариант 2

Потому что у нас уже появляется собственная framework responsibility.

Например, NodeTree сейчас гарантирует:

node visibility
nested clipping
renderer state isolation
subtree rendering

Это уже не SDL responsibility.

SDL просто предоставляет механизмы.

То есть:

SDL:
    "Вот тебе clip rectangle."


Framework:
    "Я решаю, какой clip rectangle должен получить node."

И это очень полезное различие.

То же самое:

SDL:
    "Вот SDL_Renderer."


Framework:
    "Я решаю, когда и в каком состоянии его использовать."

Поэтому RendererStateScope — это не abstraction ради abstraction.

Это framework-owned rendering policy.

4. Я бы поэтому немного переименовал саму концепцию

Я бы не думал о ней сейчас как:

Backend abstraction

Я бы думал:

Framework Rendering Contract
            ↓
        SDL3 adapter

То есть у нас есть:

Framework rendering semantics
visibility
clipping
ordering
state isolation
geometry
future opacity
future transforms

и:

SDL implementation
SDL_Renderer
SDL_RenderLine
SDL_RenderRect
SDL_SetRenderClipRect
...

Это более здоровое разделение.

5. Где тогда находится Node::draw(SDL_Renderer*)?

Вот здесь появляется архитектурный вопрос.

Сейчас:

virtual void draw(SDL_Renderer *renderer);

означает:

Node знает, что backend = SDL.

Это вполне допустимо если SDL является сознательно постоянным backend-ом.

Но если мы считаем:

«Возможно, через несколько лет у framework будет другой renderer»

тогда Node слишком сильно связан с SDL.

И я бы здесь не делал поспешный RenderContext.

Вместо этого стоит сначала спросить:

Нужна ли нам вообще backend independence?

Если ответ:

Нет. UI framework рассчитан на SDL3 как на фундамент.

тогда:

draw(SDL_Renderer*)

абсолютно нормален.

Это даже лучше по простоте.

6. А вот если ответ «возможно потом» — тогда другой подход

Можно разделить:

Node
  ↓
Node rendering contract

и

Node implementation
  ↓
SDL primitives

Но это не означает, что нужно прямо сейчас делать:

class IRenderBackend

Можно оставить расширяемость только на уровне framework internal boundary.

Например, концептуально:

NodeTree
   ↓
render subtree
   ↓
Node::draw()
   ↓
primitives
   ↓
SDL3

и пока primitives являются самой нижней framework-controlled границей.

У нас они уже существуют:

primitives.hpp
primitives.cpp

И это, возможно, намного более важный факт, чем кажется.

7. primitives могут быть нашим первым backend boundary

Сейчас у нас есть:

Node
 ↓
primitives
 ↓
SDL3

Это уже позволяет сделать:

primitives::rectangle(...)
primitives::line(...)
primitives::box(...)

не заставляя каждый node писать низкоуровневый SDL-код.

Но legacy/custom code всё ещё может обращаться напрямую к SDL.

Поэтому возникает вопрос:

Хотим ли мы сделать primitives официальным framework-owned rendering boundary?

То есть:

Node/component
      ↓
Framework rendering primitives
      ↓
SDL3

А не:

Node/component
      ↓
SDL3 напрямую

На мой взгляд, это гораздо интереснее для E, чем вводить IRenderBackend.

8. И здесь появляется очень хороший критерий

Я бы спросил:

Что именно framework должен гарантировать независимо от SDL?

Например:

Framework guarantees
clip is nested correctly
renderer state is restored
draw order is deterministic
actual geometry is used
node cannot corrupt sibling renderer state
SDL provides
line
rect
texture
render target
blend
scale
...

То есть SDL остаётся механизмом, а framework — владельцем semantics.

Это очень чистая модель.

9. Что тогда означает external/SDL3

С твоей delivery model:

ui_framework/
    external/
        SDL3/
        SDL3_image/
        SDL3_ttf/

я бы считал это dependency packaging, а не частью runtime architecture.

То есть:

external/
    = supplied toolchain dependencies


src/
    = framework implementation


include/
    = framework public contract

Это важно, потому что я бы не переносил SDL под:

include/ui_framework/backend/

или:

src/backend/

если это просто внешний dependency.

10. Но есть один вопрос, который я бы обязательно обсудил

Ты сказал:

framework получает нужный ему контекст SDL.

Сейчас это SDL_Renderer*.

Но возможно позже framework будет требовать ещё:

SDL_Window*
SDL_Texture*
SDL_Surface*
TTF_Font*

И тогда вопрос:

Что именно означает «контекст SDL», который application передаёт framework?

Сейчас у нас:

runFrame(float dt, SDL_Renderer *renderer)

Это нормально.

Но если rendering system начнёт использовать:

renderer
window
font
textures
render targets

то мы можем постепенно получить:

runFrame(
    dt,
    renderer,
    window,
    ...
)

И вот здесь уже может появиться настоящая необходимость в RenderContext.

Не сейчас. А только когда concrete API начинает этого требовать.

11. Поэтому я бы пока предложил такой E contract
                    APPLICATION
                        │
                        │ owns
                        ▼
                  SDL lifecycle
                        │
                        ▼
                  SDL_Renderer*
                        │
                        ▼
              ┌─────────────────┐
              │   UI Framework  │
              │                 │
              │ NodeTree        │
              │ rendering state │
              │ clipping        │
              │ ordering        │
              │ primitives      │
              └────────┬────────┘
                       │
                       ▼
                      SDL3

И отдельно:

external/SDL3
external/SDL3_image
external/SDL3_ttf


        =
framework-supplied dependencies

а не:

framework-owned SDL runtime
12. Что я бы сейчас НЕ делал

Не вводил бы:

IRenderBackend
RenderContext
BackendFactory
RendererInterface
SDLBackend class
OpenGLBackend
VulkanBackend

пока у нас нет второго concrete backend или хотя бы конкретного требования, которое нельзя выразить текущим SDL contract.

И это не потому, что abstraction «плохая».

А потому что сейчас она будет скрывать очень маленький и понятный dependency:

SDL_Renderer*

за большим количеством типов.

13. Но я бы сделал одну важную вещь в E

Я бы явно зафиксировал ownership boundary:

Framework provides:
    SDL3 + SDL3_image + SDL3_ttf binaries/headers/libs


Application owns:
    SDL_Init / SDL_Quit
    SDL_Window
    SDL_Renderer
    lifecycle
    renderer creation/destruction


Framework owns:
    rendering traversal
    renderer state isolation
    clipping
    primitive usage
    Node drawing contract

Это очень сильный контракт, и он предотвращает довольно много будущих ошибок.

И вот вопрос, который, на мой взгляд, стоит решить до того, как мы будем считать E окончательно завершённым

Хотим ли мы считать SDL_Renderer* частью публичного API framework или хотим, чтобы SDL был публично нужен только как dependency, а сам renderer eventually скрывался за framework-owned rendering context?

Я сейчас склоняюсь к первому варианту:

UIManager::runFrame(dt, SDL_Renderer*);

потому что он минимальный, честный и полностью соответствует текущей SDL-first архитектуре.

Но это уже действительно центральное решение E, и от него зависит, нужно ли нам вообще развивать какой-либо дополнительный backend layer.
---------------------------------
самое важное, что мы точно должны уяснить, это то, что фреймворк работает и будет работать только с sdl3 backend, остальной вопрос уже в том, как он будет с ним конкретно работать. Еще один вопрос заключается в подходе к получению этого sdl3: сейчас я использую подход, в котором фреймворк предоставляет зависимость, а клиент следит за Lifecycle самого бекенда, но помимо этого нужно рассмотреть варианты: может ли sdl3 не иметь внутри себя sdl3 зависимость, а чтобы клиент ее поставлял? В этом случае я думаю это нормально нереально сделать без абстракций, но тогда это уже не sdl3 oriented backend,а  какой-то абстрактный бекенд. Я имею в виду, можем ли мы как-то получить sdl типы без хранения зависимости sdl внутри самого фреймворка, вроде никак? Тогда получается, что у нас еще 1 вариант, в котором клиент вообще ничего не знает о sdl3 бекенде, а фреймворк сам следит на sdl3 бекендом, его жизненным циклом. В этом случае компоненты не получают sdl3 как внешний инстанс, а хранятся на уровне фреймворка и он сам прокидывает их внутрь компонент, или что-то в этом роде. В этом случае уже нужно сравнивать плюсы и минусы хранения sdl инстансов на клиенте и внутри фреймворка.

Да. И вот здесь, на мой взгляд, мы наконец дошли до настоящего смысла пункта E.

Самое важное решение уже принято:

Framework является SDL3-only framework. Другого backend у него не предполагается.

Поэтому нам не нужно обсуждать «как сделать backend-agnostic UI framework». Нужно обсуждать:

каким образом SDL3 входит в систему, кому принадлежит его runtime lifecycle и насколько глубоко SDL3 должен быть виден пользователю framework.

И это действительно три разных решения.

Сначала важное уточнение: «SDL backend» и «SDL dependency» — не одно и то же

Сейчас у нас есть одновременно:

SDL3 = backend
SDL3 = external dependency
SDL3 = часть публичного C++ API

Последний пункт особенно важен.

Сейчас Node напрямую включает:

#include <SDL3/SDL.h>

и его draw() принимает SDL_Renderer*.

А UIManager::runFrame() тоже принимает SDL_Renderer*.

Значит SDL3 сейчас не просто используется внутри framework.

SDL3 является частью public API framework.

Отсюда следует очень важный вывод:

При текущем API клиент не может полностью «не знать о SDL3».

Даже если мы somehow спрячем бинарник SDL3 внутри framework, клиент при компиляции кода, который включает Node/UIManager, всё равно сталкивается с SDL_Renderer* и SDL3/SDL.h.

Поэтому твоя мысль:

«Можно ли не поставлять SDL3 внутри framework, а чтобы клиент сам его поставлял?»

— да, технически можно, но тогда мы меняем именно dependency delivery model, а не backend architecture.

Теперь три основные модели
Вариант A — текущий
Framework
    │
    ├── supplies SDL3
    ├── supplies SDL3_image
    └── supplies SDL3_ttf
            │
            ▼
        SDL runtime


Application
    │
    ├── SDL_Init()
    ├── creates SDL_Window
    ├── creates SDL_Renderer
    │
    ▼
UIManager::runFrame(dt, renderer)

То есть:

dependency delivery → framework
runtime ownership    → application
backend              → SDL3

Это очень сильная и чистая модель.

Плюсы

Framework гарантирует:

«У меня есть известная SDL3 dependency, с которой я разрабатывался».

Application гарантирует:

«Я сам решаю, когда SDL запускается и какой renderer используется».

Это прекрасно подходит приложению, которое само использует SDL.

Например:

Application
   ├── Game
   ├── Window
   ├── Renderer
   └── UI Framework
           ↑
       uses same SDL_Renderer

Нет двух SDL renderer instances.

Нет двух lifecycle owners.

Нет скрытого окна.

Вариант B — SDL dependency поставляет клиент

Это тоже возможно.

Получается:

Framework
    │
    ├── requires SDL3
    └── uses SDL3 API
              ▲
              │
Application ──┘
    provides SDL3

То есть framework не содержит:

external/SDL3

и вместо этого говорит:

«Мне нужен SDL3 development package совместимой версии».

В чём проблема?

Не в backend.

Проблема в packaging.

Потому что сейчас framework public API уже содержит:

SDL_Renderer*

следовательно клиенту нужны как минимум SDL3 headers при compilation.

А если framework статически или динамически линкуется определённым образом, надо ещё решать binary compatibility/versioning.

То есть получается классическая модель:

Application
    ├── SDL3 headers
    ├── SDL3 libraries
    └── UI Framework
Плюс

Framework становится легче распространять.

Минус

Framework теряет гарантию:

«Я сам контролирую версию SDL3, на которой гарантированно построен».

Клиент может принести:

SDL 3.x

другой minor/patch/build variant и нам потом придётся поддерживать эту matrix.

Для проекта на твоём месте я бы считал это возможным, но не обязательно лучшим.

Вариант C — framework полностью владеет SDL runtime

Вот это уже совсем другая архитектура:

Application
      │
      ▼
UI Framework
      │
      ├── SDL_Init()
      ├── SDL_Window
      ├── SDL_Renderer
      └── SDL_Quit()

Клиент говорит примерно:

UIManager ui;
ui.run();

и framework сам решает:

когда initialize SDL;
какое окно создать;
какой renderer создать;
когда всё уничтожить.
Плюсы

Для клиента очень просто:

client
  ↓
framework
  ↓
SDL

Клиент вообще не обязан знать SDL.

Но цена здесь очень большая.

Framework теперь должен отвечать за:

window configuration
renderer configuration
fullscreen
vsync
multiple windows
resize
display
event pumping
SDL lifecycle

А это уже начинает превращать UI framework в application runtime/framework, а не только UI framework.

И это плохо сочетается с твоей целью.

Потому что chess application, например, уже может хотеть:

SDL Window
SDL Renderer
Game rendering
Input
UI Framework

и становится непонятно, кто владеет чем.

Но есть ещё очень важный промежуточный вариант

На мой взгляд, его стоит рассмотреть отдельно.

Вариант D — framework owns an SDL context, но принимает внешний renderer

Например:

Application
    │
    ├── SDL_Init
    ├── Window
    └── Renderer
            │
            ▼
       UI Framework
            │
            └── SDL-specific context

То есть framework мог бы иметь что-то вроде:

UIManager ui(renderer);

и внутри:

UIManager
    └── RendererContext
           └── SDL_Renderer*

Но сам renderer всё равно принадлежит application.

Это уже не backend abstraction в смысле:

SDL / OpenGL / Vulkan

Это просто framework-owned representation of SDL rendering state.

И вот это может стать полезным позже.

Теперь главный вопрос: нужно ли вообще скрывать SDL от клиента?

Вот тут я бы занял довольно жёсткую позицию.

У нас есть два разных желания:

Желание 1

«Framework работает только с SDL3».

и

Желание 2

«Клиент не должен знать, что framework работает с SDL3».

Эти вещи не связаны автоматически.

Можно иметь SDL3-only framework, который открыто говорит:

draw(SDL_Renderer*);
runFrame(dt, SDL_Renderer*);

и это совершенно нормальная архитектура.

А можно иметь SDL3-only framework, который скрывает SDL за:

RenderContext

но тогда мы получаем abstraction layer.

Это всё ещё SDL3-only framework.

Но abstraction становится:

UI Framework Render API
          ↓
        SDL3

а не:

UI Framework
    ↓
Generic Backend
   ↙       ↘
SDL3     Vulkan

Это важное различие.

Поэтому я бы не называл RenderContext автоматически «абстрактным backend»

Допустим, мы сделаем:

class RenderContext
{
public:
    SDL_Renderer *renderer() const;
};

Это всё ещё SDL-specific context.

Он не скрывает backend.

Он просто скрывает способ передачи SDL_Renderer* по framework.

И это может быть полезно, если со временем нам понадобится:

renderer
window
viewport
scale
render target
resource handles

Но пока у нас этого нет.

Где здесь компоненты?

Ты правильно подметил ещё один момент.

Сегодня:

NodeTree
   ↓
Node::draw(SDL_Renderer*)

и дальше framework передаёт renderer каждому node.

Это не значит, что каждый component должен хранить renderer.

Наоборот, я считаю правильным нынешний принцип:

Node
   │
   └── draw(renderer)

То есть renderer существует на время текущей операции.

Не:

Button
   └── SDL_Renderer* renderer_;

Это было бы хуже по ownership.

Если framework сам владеет renderer

Тогда получается другая схема:

UIManager
    └── SDL_Renderer*
          │
          ├── NodeTree
          │      ├── Button
          │      ├── Panel
          │      └── Modal
          │
          └── primitives

Renderer всё равно можно прокидывать вниз, не хранить в каждом node.

Поэтому ownership и propagation — две разные вещи.

Я бы вообще разделил:

WHO OWNS SDL_Renderer?

от:

WHO USES SDL_Renderer?

Framework может его использовать везде, но не обязательно владеть им.

Для нашего framework я сейчас вижу такую картину
Вариант A
             Application
                 │
        owns SDL lifecycle
                 │
          SDL_Window
                 │
          SDL_Renderer
                 │
                 ▼
          UI Framework
                 │
          NodeTree / Node
                 │
                 ▼
                SDL3

Это:

простая, честная, SDL-native архитектура.

Вариант C
             Application
                 │
                 ▼
          UI Framework
                 │
          owns SDL lifecycle
                 │
          SDL_Window
                 │
          SDL_Renderer
                 │
                 ▼
                SDL3

Это:

application-runtime architecture, а не просто UI framework.

Я бы не выбирал её для текущего проекта.

И вот ещё один очень важный момент по твоему external/SDL3

Твоя текущая структура:

ui_framework/
    external/
        SDL3/
            bin/
            include/
            lib/
            share/
        SDL3_image/
        SDL3_ttf/

хорошо соответствует варианту A.

То есть:

Framework отвечает за доступность и поставку SDL3, application отвечает за lifetime SDL objects.

Я считаю эту модель очень разумной.

Где я бы остановился сейчас

Я бы пока не менял E в коде вообще.

Сначала нужно принять два решения отдельно:

Решение №1 — dependency delivery

Из двух:

A. Framework bundles/provides SDL3
B. Client provides SDL3

Я пока голосую за A.

Причина: SDL3 является частью публичного API framework, а framework SDL3-only. Тогда bundling dependency логичен.

Решение №2 — runtime ownership

Из:

A. Application owns SDL lifecycle + renderer
B. Framework owns SDL lifecycle + renderer

Я достаточно сильно голосую за A.

Причина: framework остаётся UI framework, а не превращается в application/window runtime.

А третий вопрос я бы пока оставил открытым
SDL_Renderer*
     ↓
передаём напрямую

или

SDL_Renderer*
     ↓
SDL-specific RenderContext
     ↓
components

И вот это уже действительно хороший предмет для обсуждения E.

Но я бы не рассматривал RenderContext как средство скрыть SDL от пользователя. Если мы точно решили, что framework SDL3-only, то скрывать сам факт SDL3 необязательно.

Сейчас я бы даже сформулировал цель следующим образом:

Не сделать SDL незаметным, а сделать SDL ownership и framework rendering contract однозначными.

И в текущем source Node уже честно показывает SDL dependency через SDL_Renderer*.

На мой взгляд, сначала стоит определиться именно с A/B по ownership и delivery, а уже потом решать, нужен ли нам SDL-specific RenderContext.
