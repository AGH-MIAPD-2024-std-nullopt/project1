function addInput(type) {
  const container = document.getElementById(`${type}Inputs`);
  const newRow = document.createElement('div');
  newRow.className = 'input-row';
  newRow.innerHTML = `
      <input type="text" placeholder="Nazwa ${type === 'category' ? 'kategorii' : 'alternatywy'}">
      <button onclick="this.parentNode.remove()">-</button>
  `;
  container.appendChild(newRow);
}

function getInputValues(type) {
  const inputs = document.querySelectorAll(`#${type}Inputs input`);
  return Array.from(inputs)
      .map(input => input.value.trim())
      .filter(value => value !== '');
}

function createComparisonMatrix(title, elements, matrixId) {
  const matrixContainer = document.createElement('div');
  matrixContainer.className = 'comparison-section';
  matrixContainer.innerHTML = `<h3>${title}</h3>`;

  const matrix = document.createElement('div');
  matrix.id = matrixId;
  matrix.className = 'comparison-matrix';
  matrix.style.gridTemplateColumns = `repeat(${elements.length + 1}, 1fr)`;

  // Nagłówki kolumn
  const headerRow = document.createElement('div');
  headerRow.className = 'matrix-cell';
  headerRow.textContent = '';
  matrix.appendChild(headerRow);
  elements.forEach(el => {
      const cell = document.createElement('div');
      cell.className = 'matrix-cell';
      cell.textContent = el;
      matrix.appendChild(cell);
  });

  // Tworzenie macierzy
  elements.forEach((rowEl, rowIndex) => {
      const rowHeader = document.createElement('div');
      rowHeader.className = 'matrix-cell';
      rowHeader.textContent = rowEl;
      matrix.appendChild(rowHeader);

      elements.forEach((colEl, colIndex) => {
          const cell = document.createElement('div');
          cell.className = 'matrix-cell';
          if (rowIndex === colIndex) {
              cell.textContent = '1';
          } else if (rowIndex < colIndex) {
              const select = document.createElement('select');
              select.id = `${matrixId}_${rowEl}_${colEl}`;
              [
                  { value: 0.111, label: '1/9' }, { value: 0.125, label: '1/8' }, { value: 0.143, label: '1/7' },
                  { value: 0.167, label: '1/6' }, { value: 0.2, label: '1/5' }, { value: 0.25, label: '1/4' },
                  { value: 0.333, label: '1/3' }, { value: 0.5, label: '1/2' }, { value: 1, label: '1' },
                  { value: 2, label: '2' }, { value: 3, label: '3' }, { value: 4, label: '4' },
                  { value: 5, label: '5' }, { value: 6, label: '6' }, { value: 7, label: '7' },
                  { value: 8, label: '8' }, { value: 9, label: '9' }
              ].forEach(option => {
                  const opt = document.createElement('option');
                  opt.value = option.value;
                  opt.textContent = option.label;
                  select.appendChild(opt);
              });
              cell.appendChild(select);
          }
          matrix.appendChild(cell);
      });
  });

  matrixContainer.appendChild(matrix);
  return matrixContainer;
}

function createComparisonMatrices() {
  const categories = getInputValues('category');
  const alternatives = getInputValues('alternative');

  if (categories.length < 2 || alternatives.length < 2) {
      alert('Musisz podać co najmniej 2 kategorie i 2 alternatywy');
      return;
  }

  // disable all buttons and textboxes recusively inside configSection
  var configSection = document.getElementById('configSection');
  var elements = configSection.getElementsByTagName('*');
  for (var i = 0; i < elements.length; i++) {
      elements[i].disabled = true;
  }

  const setup = { criteria: [], alternatives: [] }; 
  const results = { criteriaMatrix: {}, alternativeMatrices: {} };

  // Eksport kategorii
  categories.forEach((category, index) => {
      setup.criteria.push(category);
  });

  // Eksport alternatyw
  alternatives.forEach((alternative, index) => {
      setup.alternatives.push(alternative);
  });
  fetch('/submitSetup?data=' + encodeURIComponent(JSON.stringify(setup)), {
      method: 'GET',
      headers: {
          'Content-Type': 'application/json'
      }
  })
  .then(response => response.json())
  .then(data => {
      console.log('Setup: Success:', data);
  })
  .catch((error) => {
      alert('Wystąpił błąd podczas uaktualniania ustawień: ' + error);
  });

  // Usuwanie poprzednich macierzy
  const comparisonSections = document.getElementById('comparisonSections');
  comparisonSections.innerHTML = '';

  // Macierz kategorii
  const categoryMatrixContainer = createComparisonMatrix('Porównanie kategorii', categories, 'category_matrix');
  comparisonSections.appendChild(categoryMatrixContainer);

  // Macierze dla alternatyw w każdej kategorii
  categories.forEach(category => {
      const altMatrixContainer = createComparisonMatrix(
          `Porównanie alternatyw w kategorii: ${category}`, 
          alternatives, 
          `alternatives_matrix_${category}`
      );
      comparisonSections.appendChild(altMatrixContainer);
  });

  // Eksport wyników
  const exportButton = document.createElement('button');
  exportButton.textContent = 'Wyślij odpowiedzi';
  exportButton.style = 'float: left; margin-bottom: 20px;';
  exportButton.onclick = exportResults;
  comparisonSections.appendChild(exportButton);

  // Rezultaty
  const resultButton = document.createElement('button');
  resultButton.textContent = 'Wygeneruj wyniki';
  resultButton.style = 'float: right; margin-bottom: 20px;';
  resultButton.onclick = function() {
      window.location.href = '/results';
  };
  comparisonSections.appendChild(resultButton);
}

function exportResults() {
  const criteria = getInputValues('category');
  const alternatives = getInputValues('alternative');
  const results = { criteriaMatrix: {}, alternativeMatrices: {} };

  // Eksport macierzy kategorii
  criteria.forEach(row => {
      results.criteriaMatrix[row] = {};
      criteria.forEach(col => {
          if(row === col) {
              results.criteriaMatrix[row][col] = 1;
              return;
          }
          const cellId = `category_matrix_${row}_${col}`;
          const select = document.getElementById(cellId);
          if(select) {
              results.criteriaMatrix[row][col] = select.value;
          }                    
      });
  });

  // Eksport macierzy alternatyw
  criteria.forEach(category => {
      results.alternativeMatrices[category] = {};
      alternatives.forEach(row => {
          results.alternativeMatrices[category][row] = {};
          alternatives.forEach(col => {
              if(row === col) {
                  results.alternativeMatrices[category][row][col] = 1;
                  return;
              }
              const cellId = `alternatives_matrix_${category}_${row}_${col}`;
              const select = document.getElementById(cellId);
              if(select) {
                  results.alternativeMatrices[category][row][col] = select.value;
              }
          });
      });
  });

  fetch('/submit?data=' + encodeURIComponent(JSON.stringify(results)), {
      method: 'GET',
      headers: {
          'Content-Type': 'application/json'
      }
  })
  .then(response => response.json())
  .then(data => {
      alert('Wyniki zostały wysłane pomyślnie.');
  })
  .catch((error) => {
      alert('Wystąpił błąd podczas wysyłania wyników: ' + error);
  });
}