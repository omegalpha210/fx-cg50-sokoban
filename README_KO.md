# CASIO fx-CG50 소코반

![새 SOKOBAN 앱 아이콘](assets/icon-uns.png)

C + fxSDK/gint 네이티브 애드인입니다. 네 그룹·60개 레벨, 레벨별 자동 저장,
최근 5회 UNDO, INIT 확인, MENU 및 SHIFT+AC/ON 체크포인트를 지원합니다.
**v0.1.0-beta.3 / HARDWARE RETEST REQUIRED.** 시스템의 자동 종료·백라이트
시간 설정을 읽도록 연결하고, 플레이어를 파란 마름모로 바꾸었습니다.
저장 실패 후 복귀와 버튼 안내도 개선했습니다. 새 절전 연동의 실제 밝기와
전원 복귀는 계산기에서 재시험해야 합니다.

![실제 공통 렌더러 화면](docs/public-captures/overview.png)

공개 플레이·완료·확인 화면은 프로젝트가 직접 작성한 테스트 맵을 실제
렌더러로 그린 것입니다. 원본 60개 맵이나 실기 촬영 화면은 아닙니다.

## 공개 범위와 설치

[GitHub 베타](https://github.com/omegalpha210/fx-cg50-sokoban/releases/tag/v0.1.0-beta.3)는
**소스 전용**입니다. 원본 맵의 재배포 허가가 확인되지 않아 원시 맵,
생성된 맵 데이터, 해당 맵 화면, 맵을 포함한 `.g3a`는 공개하지 않습니다.
자체 코드는 [MIT](LICENSE), 외부 자료는 [별도 고지](THIRD_PARTY_NOTICES.md)를
따릅니다. MIT를 원본 맵에 적용하지 않습니다.

기존 fxSDK/gint/SH 컴파일러 환경을 설정한 뒤 직접 고정 자료를 확보합니다.

```sh
python3 tools/fetch_maps.py
python3 tools/import_maps.py
bash tools/build.sh
bash tools/test.sh
```

일반 빌드는 네트워크에서 맵을 갱신하지 않습니다. 로컬 생성된
`dist/SOKOBAN.g3a`를 USB 저장장치 모드로 계산기 저장 메모리에 복사하고,
안전하게 연결 해제 후 CASIO MAIN MENU에서 실행합니다.
`dist/SHA256SUMS.txt`로 파일을 확인할 수 있습니다.
자세한 환경 설정은 [DEVELOPMENT.md](docs/DEVELOPMENT.md)를 참고하세요.

## 조작

- 메뉴 LEFT/RIGHT: 이전·다음 번호. 행 끝에서 다음 행으로 이어지고 전체 페이지가 순환합니다.
- 메뉴 UP/DOWN: 같은 열에서 위·아래로 순환합니다.
- Main 숫자 1~4: 그룹 즉시 진입. EXE/F6 OPEN: 선택 항목 열기.
- 플레이 방향키: 이동 또는 상자 한 개 밀기.
- F1 INIT: 현재 레벨 초기화 확인. EXE 확인, EXIT 취소.
- F2 UNDO: 성공한 이동을 최근 5회까지 복원. 길게 눌러도 한 번만 실행합니다.
- F5 LEVEL- / F6 LEVEL+: 저장 후 전체 번호 기준 이전·다음 레벨. 1↔60 순환 없음.
- EXIT: 플레이 저장 후 레벨 선택으로 복귀. 레벨 선택에서는 Main, Main에서는 머뭅니다.
- MENU: 저장 후 실제 CASIO MAIN MENU로 전환합니다.
- SHIFT+AC/ON: 변경된 진행을 한 번 저장 시도한 뒤 전원을 끕니다. 저장 실패로 전원 끄기를 막지 않습니다.

BASIC 1~15 / INTERMEDIATE 16~30 / ADVANCED 31~45 / MASTER 46~60이며,
처음부터 모두 선택할 수 있습니다. 이 분류를 원본 난이도 순서라고 주장하지 않습니다.

각 레벨의 플레이어·상자·카운터·UNDO를 독립 보존합니다. 클리어 기록은 INIT과
재도전에도 유지합니다. 보통 이동은 RAM만 갱신하고, 완료·INIT·EXIT·레벨 이동·
MENU·변경 후 OFF에서 체크포인트를 만듭니다. 강제 전원 차단 직전의 미저장
이동까지 보장하지 않습니다. 전원을 켰을 때 gint가 중단 실행으로 돌아오면 RAM을
유지하고, 앱을 새로 실행하면 Main부터 시작하여 저장된 레벨 진행을 복원합니다.

## 이번 변경과 검증

- SYSTEM의 Auto Power Off 10/60분, Backlight Duration 30초/1분/3분을 시작 및
  MENU/ON 복귀 시 읽습니다. 사용하지 않는 키와 길게 누른 키도 대기 시간을 갱신합니다.
  백라이트는 CG50 절전 단계 0을 요청하고, 입력 시 설정된 밝기로 복원합니다.
  OS 설정 자체를 바꾸지 않습니다. 실제 LCD 단계가 OS와 같은지는 실기 확인 대상입니다.
- 자동 OFF도 모든 화면에서 저장 후 종료합니다. 저장 실패는 종료를 막지 않으며,
  ON 복귀 시 SAVE FAILED에서 재시도할 수 있습니다.
- 플레이어는 9~19픽셀 모두 파란 마름모입니다. 목표의 작은 어두운 점과 색·크기·형태를
  구분하고 HUD에 YOU 범례를 넣었습니다. UNDO가 없거나 레벨 끝이면 버튼이 흐리게 표시됩니다.
- 확인 창은 관련 없는 F-key 안내를 숨깁니다. INIT은 RESTART LEVEL?로 명확히 표시합니다.
  저장 오류 화면에서도 MENU를 누르고 재시도하거나 F6으로 저장 없이 나갈 수 있습니다.
- 파일 I/O 105개 호출 지점의 실패, 10만 입력 이벤트, 전체 맵 이동·UNDO 및
  손상 저장 데이터를 검사했습니다. 상세 조건은 [안정성·오류 검증](docs/STABILITY_KO.md)에 있습니다.

![9~19픽셀 플레이어, 3배 확대](docs/public-captures/player-sizes-3x.png)

게임 규칙·맵·저장 형식·beta.2의 런처 아이콘은 유지됩니다.

앞선 beta.1에서 플레이의 24픽셀 제목줄을 없애고 HUD를 올렸습니다. 보드 공간은
268×172에서268×196으로 커졌으며 48개 레벨의 타일이 확대되었습니다.
최소 타일은8→9픽셀입니다. Main/레벨 제목과 하단 F-key는 유지합니다.

[검증 결과](docs/ACCEPTANCE.md), [60개 크기 비교](docs/LAYOUT_AUDIT.md),
[아이콘 비교](docs/ICON_AUDIT.md), [전원 처리](docs/POWER.md),
[실기 재시험](docs/HARDWARE_RETEST.md), [출처·라이선스](docs/ASSET_PROVENANCE.md)를
확인하세요. 원본 맵 구조 검증은 60개 모두 해결했다는 의미가 아닙니다.
