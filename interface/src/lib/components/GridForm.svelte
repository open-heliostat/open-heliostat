<style>
	.grid-form {
	  grid-template-columns: [labels] auto [controls] 1fr;
	}
	/* .grid-form {
	  display: grid;
	  grid-template-columns: [labels] auto [controls] 1fr;
	  grid-auto-flow: row;
	}
	.grid-form label  {
	  grid-column: labels;
	  grid-row: auto;
	}
	.grid-form input {
	  grid-column: controls;
	  grid-row: auto;
	} */
</style>

<div class="w-full grid grid-flow-row grid-form items-center">
    <slot></slot>
</div>