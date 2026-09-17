# CASIO fx-CG50 소코반

![새 SOKOBAN 앱 아이콘](assets/icon-uns.png)

C + fxSDK/gint 네이티브 애드인입니다. 네 그룹·60개 레벨, 레벨별 자동 저장,
최근 5회 UNDO, INIT 확인, MENU 및 SHIFT+AC/ON 체크포인트를 지원합니다.
**v0.1.0-beta.2 / HARDWARE RETEST REQUIRED.** 이번 버전은 앱 아이콘을
10×10픽셀 공통 격자로 다시 디자인하고 아래쪽 22픽셀 여백을 확보했습니다.
기존 기본 플레이는 사용자 확인 상태이며 새 아이콘과 남은 전원·화면 검증은 실기 재시험이 필요합니다.

![실제 공통 렌더러 화면](docs/public-captures/overview.png)

공개 플레이·완료·확인 화면은 프로젝트가 직접 작성한 테스트 맵을 실제
렌더러로 그린 것입니다. 원본 60개 맵이나 실기 촬영 화면은 아닙니다.

## 공개 범위와 설치

[GitHub 베타](https://github.com/omegalpha210/fx-cg50-sokoban/releases/tag/v0.1.0-beta.2)는
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

새 아이콘은 벽과 상자의 외곽을 같은 10×10픽셀 셀에 맞추고,
검은 플레이어→주황 상자→작은 목표를 배치했습니다. 92×64 캔버스에서
그림 경계는 (6,2)..(85,41), 아래 여백은 17→22픽셀입니다.
선택 상태에서도 그림 자체는 동일합니다. 상용·CASIO 스프라이트나 내부 제목은 없습니다.
게임 엔진·맵·저장·UI·조작·전원 처리는 beta.1에서 변경하지 않았습니다.

앞선 beta.1에서 플레이의 24픽셀 제목줄을 없애고 HUD를 올렸습니다. 보드 공간은
268×172에서268×196으로 커졌으며 48개 레벨의 타일이 확대되었습니다.
최소 타일은8→9픽셀입니다. Main/레벨 제목과 하단 F-key는 유지합니다.

[검증 결과](docs/ACCEPTANCE.md), [60개 크기 비교](docs/LAYOUT_AUDIT.md),
[아이콘 비교](docs/ICON_AUDIT.md), [전원 처리](docs/POWER.md),
[실기 재시험](docs/HARDWARE_RETEST.md), [출처·라이선스](docs/ASSET_PROVENANCE.md)를
확인하세요. 원본 맵 구조 검증은 60개 모두 해결했다는 의미가 아닙니다.
