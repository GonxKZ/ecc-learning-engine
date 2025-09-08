#!/usr/bin/env bash
set -euo pipefail
set +H  # evita expansión de '!' (npm ERR!, etc.)

# ==== Config ==========================
REPO="${REPO:-}"                    # si no se define, se detecta con gh
LIMIT="${LIMIT:-200}"               # nº máx de runs a revisar
WORKFLOW="${WORKFLOW:-}"            # filtrar por workflow .yml, opcional
SINCE="${SINCE:-}"                  # limitar por fecha (YYYY-MM-DD), opcional
OUTFILE="${OUTFILE:-all-errors.txt}" # salida única

# Patrones de error (ajústalos si quieres)
PATTERNS="${PATTERNS:-error|failed|exception|traceback|panic|fatal|npm ERR!|AssertionError|pytest|segmentation fault|CMake Error|clang: error|ld: error|Undefined reference}"
# =====================================

command -v gh >/dev/null 2>&1 || { echo "Falta GitHub CLI (gh)"; exit 1; }
gh auth status >/dev/null || { echo "gh no autenticado"; exit 1; }

# Detectar repo si no viene por env
if [[ -z "$REPO" ]]; then
  REPO="$(gh repo view --json nameWithOwner -q .nameWithOwner)"
fi

echo "Repo: $REPO"
echo "Límite: $LIMIT"
[[ -n "$WORKFLOW" ]] && echo "Workflow filtrado: $WORKFLOW"
[[ -n "$SINCE"   ]] && echo "Desde: $SINCE"
echo "Patrones (grep -Ei): $PATTERNS"
echo "Salida: $OUTFILE"
echo

# Construir flags para gh run list
LIST_FLAGS=(-R "$REPO" --json databaseId,name,headSha,createdAt,conclusion,url -L "$LIMIT")
[[ -n "$WORKFLOW" ]] && LIST_FLAGS+=(-w "$WORKFLOW")
[[ -n "$SINCE"   ]] && LIST_FLAGS+=(--created ">=$SINCE")

# Obtener runs fallidos
mapfile -t RUNS < <(gh run list "${LIST_FLAGS[@]}" \
  --jq '.[] | select(.conclusion=="failure") | [.databaseId, .name, .headSha, .createdAt, .url] | @tsv')

: > "$OUTFILE"
if [[ ${#RUNS[@]} -eq 0 ]]; then
  echo "No hay runs fallidos." | tee -a "$OUTFILE"
  exit 0
fi

# Procesar runs
for ROW in "${RUNS[@]}"; do
  IFS=$'\t' read -r runid name headsha createdAt url <<< "$ROW"

  # Ruta del workflow YAML (REST)
  yaml_path="$(gh api "repos/$REPO/actions/runs/$runid" --jq .path 2>/dev/null || echo "desconocido")"

  # Extraer logs (preferente: pasos fallidos; si no, todo)
  {
    if gh run view -R "$REPO" "$runid" --log-failed 2>/dev/null; then
      :
    else
      gh run view -R "$REPO" "$runid" --log 2>/dev/null || true
    fi
  } | grep -Ein "$PATTERNS" | sed 's/\r$//' > "/tmp/errlines.$runid.txt" || true

  # Escribir sección
  {
    echo "================================================================"
    echo "Run ID:        $runid"
    echo "Workflow:      $name"
    echo "YAML:          $yaml_path"
    echo "Commit:        $headsha"
    echo "Created:       $createdAt"
    echo "Run URL:       $url"
    echo "----------------------------------------------------------------"
    if [[ -s "/tmp/errlines.$runid.txt" ]]; then
      cat "/tmp/errlines.$runid.txt"
    else
      echo "(Sin coincidencias de PATTERNS en este run — ajusta PATTERNS si es necesario.)"
    fi
    echo
  } >> "$OUTFILE"

  rm -f "/tmp/errlines.$runid.txt" 2>/dev/null || true
done

echo ">> Generado: $OUTFILE"
