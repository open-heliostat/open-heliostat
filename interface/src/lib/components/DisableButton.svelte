<script lang="ts">
	import Disable from '~icons/tabler/alert-triangle-off';
	import Button from './Button.svelte';
    export let onClick : () => void;
</script>

<Button
    label="Disable"
    {onClick}
    >
    <Disable class="mr-2 h-5 w-5" />
</Button>